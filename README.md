# Wio Terminal Game Console Project

[English](README.md) | [简体中文](README.zh-CN.md)

## 📋 Project Overview

### What problem does it solve
A portable game console built on the Seeed Wio Terminal, with 4 classic games and a sensor toolbox:
- **Snake**: classic arcade game
- **Sokoban**: puzzle game
- **Tetris**: classic falling-block game
- **Breakout**: paddle-and-ball brick breaker
- **Sensor tools**: light, accelerometer, IR, buzzer, microphone

### Who is it for
- **Arduino beginners**: learning embedded development and game logic
- **Educators**: programming classes and electronics prototyping
- **Retro-game fans**: classic games in a portable form

### Value
- Built on the mature Wio Terminal ecosystem — no extra hardware required
- Open source and customizable; supports more games and features
- Great for maker education, STEM teaching and DIY showcases

---

## 🔧 Hardware

### Core hardware
| Component | Model | Qty | Notes |
|-----------|-------|-----|-------|
| Main board | Seeed Wio Terminal | 1 | ATSAMD51P19, ARM Cortex-M4F @ 120MHz |
| Display | 2.4" TFT LCD | 1 | 320×240, ILI9341 driver |

### Built-in input devices
| Device | Pins | Function |
|--------|------|----------|
| 5-way joystick | WIO_5S_UP/DOWN/LEFT/RIGHT/PRESS | Game control |
| Top button A | WIO_KEY_A | Pause / resume |
| Top button B | WIO_KEY_B | Restart |
| Top button C | WIO_KEY_C | Back to menu |

### Built-in sensors (tools mode)
| Sensor | Pin / interface | Purpose |
|--------|-----------------|---------|
| Light sensor | WIO_LIGHT | Ambient light detection |
| 3-axis accelerometer | I2C (LIS3DHTR) | Motion detection |
| IR emitter | WIO_IR | IR signal emission |
| Buzzer | WIO_BUZZER | Audio output |
| Microphone | WIO_MIC | Sound input |

### Dimensions / specs
| Parameter | Value |
|-----------|-------|
| Screen resolution | 320 × 240 px |
| Tetris playfield | 10×20 cells, 11px each |
| Snake playfield | 20×20 cells, 10px each |
| Sokoban playfield | 10×8 cells, 28px each |
| Breakout | 8×5 brick array |

---

## 📐 Technical documentation

### Development environment

#### 1. Arduino IDE setup
- **IDE version**: Arduino IDE 2.x or 1.8.x
- **Board**: Seeed Wio Terminal
- **Board core**: Seeed SAMD Boards (install via the Boards Manager)

#### 2. Dependencies
```cpp
#include <Seeed_GFX.h>      // display graphics library
#include <Wire.h>           // I2C library
```

#### 3. Compile settings
- **Upload speed**: 115200
- **Debug port**: SerialUSB
- **Optimization**: default

### Code architecture

#### Main flow
```
setup()
  ├─ init serial (SerialUSB @ 115200)
  ├─ configure input pins (joystick + top buttons)
  └─ draw main menu

loop()
  ├─ dispatch by currentGame state
  ├─ MENU: update menu selection
  ├─ SNAKE: run the Snake game
  ├─ SOKOBAN: run the Sokoban game
  ├─ TETRIS: run the Tetris game
  ├─ BREAKOUT: run the Breakout game
  └─ TOOLS: run the sensor tools
```

#### Game state management
```cpp
enum GameType { MENU, SNAKE, SOKOBAN, TETRIS, BREAKOUT, TOOLS };
GameType currentGame = MENU;
```

### Core game logic

#### 1. Snake
- **Playfield**: 20×20
- **Move speed**: 150 ms/step
- **Partial redraw**: only the head and tail are repainted
- **Collision**: hitting a wall or yourself ends the game

#### 2. Sokoban
- **Playfield**: 10×8
- **Levels**: 3
- **Partial redraw**: only the cells the player/box moves through are repainted
- **Win condition**: push every box onto a target

#### 3. Tetris
- **Playfield**: 10×20
- **Piece types**: 7 standard tetrominoes (I, O, T, S, Z, J, L)
- **Drop speed**: 500 ms/step
- **Partial redraw**: the old position and shape are recorded; only the changed region is repainted
- **Scoring**: 100 points per cleared line

#### 4. Breakout
- **Brick array**: 8 columns × 5 rows
- **Partial redraw**: only the ball's and paddle's movement regions are repainted
- **Collision**: ball vs bricks, paddle and walls

### Partial-redraw optimization

All games use a partial-redraw scheme to avoid flicker from full-screen repaints:

```cpp
// Tetris example
void drawTetrisGame() {
  if (tetrisFirstDraw) {
    // First draw: full-screen refresh
    display.fillScreen(TFT_BLACK);
    drawAllFixedPieces();
    tetrisFirstDraw = false;
  } else {
    // Partial redraw: clear only the old position
    clearOldPiece(lastPieceX, lastPieceY, lastPieceShape);
    // Draw the new position
    drawCurrentPiece(currentPieceX, currentPieceY, currentPiece);
  }
  // Update the recorded state
  lastPieceX = currentPieceX;
  lastPieceY = currentPieceY;
  copyPiece(lastPieceShape, currentPiece);
}
```

### Input handling

#### Joystick
```cpp
bool readJoystick(int& dirX, int& dirY, bool& press) {
  // debounce (150 ms)
  if (millis() - lastDebounceTime < debounceDelay) return false;

  // detect the 5 directions
  if (digitalRead(PIN_UP) == LOW) { dirX=0; dirY=-1; ... }
  // ... other directions
}
```

#### Top buttons
```cpp
TopKey readTopButtons() {
  // debounce (200 ms)
  // A: pause / resume
  // B: restart
  // C: back to menu
}
```

---

## 🔍 Troubleshooting

### Known issues and fixes

#### 1. Tetris flickering
**Symptom**: heavy flicker while pieces fall; motion trails left behind

**Root cause**:
- `drawTetrisGame()` is called every frame even when no piece has moved
- After rotation `currentPiece` changes, but the old shape is used when clearing the previous piece
- After a piece locks, the old-position record is not cleared, so settled pieces get erased by mistake

**Fix**:
- Added `lastPieceShape[4][4]` to remember the previous shape
- Repaint only when a piece moves / rotates / drops
- Set `tetrisFirstDraw = true` after a piece locks to trigger a full redraw

#### 2. Sokoban level 2 unsolvable
**Symptom**: a box gets pushed into a corner and can no longer move

**Root cause**: poor initial map layout — box position causes a deadlock

**Fix**: adjusted box positions to make the level solvable

#### 3. Sokoban walls flickering
**Symptom**: every move repaints the whole map, making walls flicker

**Fix**: implemented partial redraw — only the cells the player/box move through are repainted

#### 4. Tetris score cut off
**Symptom**: the score is truncated at the bottom of the screen

**Fix**: moved the score to the right side of the screen (X=260, Y=60)

### Compile / upload issues

| Issue | Fix |
|-------|-----|
| `Seeed_GFX` not found | Install Seeed Arduino GFX from the Library Manager |
| Upload fails | Check the USB connection and select the correct serial port |
| Blank screen | Confirm the `WIO_TERMINAL_PRODUCT` init code is correct |

---

## 📁 Project structure

```
sketch_aug10a/
├── sketch_aug10a.ino          # main sketch
├── README.md                  # this file (English)
├── README.zh-CN.md            # 简体中文版本
└── PROJECT_DOCUMENTATION.md   # original Chinese technical documentation
```

## 🚀 Quick start

1. **Connect**: USB-C from the Wio Terminal to your computer
2. **Open**: open `sketch_aug10a.ino` in the Arduino IDE
3. **Select board**: Tools → Board → Seeed Wio Terminal
4. **Upload**: click Upload
5. **Play**: you'll see the main menu after reboot; pick a game with the joystick

## 🎮 Controls

| Action | Control |
|--------|---------|
| Menu selection | Joystick up / down |
| Confirm selection | Joystick press |
| In-game control | Joystick direction |
| Pause / resume | Top button A |
| Restart | Top button B |
| Back to menu | Top button C |

---

## 📝 Version history

| Version | Date | Notes |
|---------|------|-------|
| 1.0 | 2026-08-12 | Initial release: 4 games and the sensor tools |

## 📄 License

Built from open-source code; free to modify and redistribute.