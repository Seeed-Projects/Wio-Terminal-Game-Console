# Wio Terminal 游戏机项目文档

[English](README.md) | [简体中文](README.zh-CN.md)

## 📋 项目概览

### 解决什么问题
本项目基于 Seeed Wio Terminal 开发板，实现了一个便携式游戏机，包含4款经典游戏和传感器工具集：
- **贪吃蛇**：经典街机游戏
- **推箱子**：益智解谜游戏
- **俄罗斯方块**：经典下落方块游戏
- **打砖块**：弹球打砖块游戏
- **传感器工具**：光线、加速度、红外、蜂鸣器、麦克风

### 为谁设计
- **Arduino 初学者**：学习嵌入式开发和游戏逻辑
- **教育工作者**：用于编程教学和电子原型设计
- **复古游戏爱好者**：体验经典游戏的便携版本

### 市场价值
- 基于成熟的 Wio Terminal 生态，无需额外硬件
- 开源可定制，支持扩展更多游戏和功能
- 适合创客教育、STEM 教学和DIY项目展示

---

## 🔧 硬件清单

### 核心硬件
| 硬件 | 型号 | 数量 | 说明 |
|------|------|------|------|
| 主控板 | Seeed Wio Terminal | 1 | ATSAMD51P19, ARM Cortex-M4F @ 120MHz |
| 显示屏 | 2.4" TFT LCD | 1 | 320×240 分辨率，ILI9341 驱动 |

### 内置输入设备
| 设备 | 引脚 | 功能 |
|------|------|------|
| 5向摇杆 | WIO_5S_UP/DOWN/LEFT/RIGHT/PRESS | 游戏控制 |
| 顶部按键A | WIO_KEY_A | 暂停/继续 |
| 顶部按键B | WIO_KEY_B | 重新开始 |
| 顶部按键C | WIO_KEY_C | 返回菜单 |

### 内置传感器（工具模式）
| 传感器 | 引脚/接口 | 用途 |
|--------|-----------|------|
| 光线传感器 | WIO_LIGHT | 环境光检测 |
| 3轴加速度计 | I2C (LIS3DHTR) | 运动检测 |
| 红外发射器 | WIO_IR | 红外信号发射 |
| 蜂鸣器 | WIO_BUZZER | 声音输出 |
| 麦克风 | WIO_MIC | 声音输入检测 |

### 整体尺寸规格
| 参数 | 数值 |
|------|------|
| 屏幕分辨率 | 320 × 240 像素 |
| 游戏区域（俄罗斯方块） | 10×20 格，每格 11px |
| 游戏区域（贪吃蛇） | 20×20 格，每格 10px |
| 游戏区域（推箱子） | 10×8 格，每格 28px |
| 游戏区域（打砖块） | 8×5 砖块阵列 |

---

## 📐 技术文档

### 开发环境配置

#### 1. Arduino IDE 设置
- **IDE版本**：Arduino IDE 2.x 或 1.8.x
- **开发板**：Seeed Wio Terminal
- **核心库**：Seeed SAMD Boards (通过板管理器安装)

#### 2. 依赖库
```cpp
#include <Seeed_GFX.h>      // 显示图形库
#include <Wire.h>           // I2C 通信库
```

#### 3. 编译设置
- **上传速度**：115200
- **调试端口**：SerialUSB
- **优化级别**：默认

### 代码架构

#### 主程序流程
```
setup()
  ├─ 初始化串口 (SerialUSB @ 115200)
  ├─ 配置输入引脚 (摇杆 + 顶部按键)
  └─ 绘制主菜单

loop()
  ├─ 根据 currentGame 状态分发
  ├─ MENU: 更新菜单选择
  ├─ SNAKE: 运行贪吃蛇游戏
  ├─ SOKOBAN: 运行推箱子游戏
  ├─ TETRIS: 运行俄罗斯方块
  ├─ BREAKOUT: 运行打砖块游戏
  └─ TOOLS: 运行传感器工具
```

#### 游戏状态管理
```cpp
enum GameType { MENU, SNAKE, SOKOBAN, TETRIS, BREAKOUT, TOOLS };
GameType currentGame = MENU;
```

### 核心游戏逻辑

#### 1. 贪吃蛇游戏
- **网格大小**：20×20
- **移动速度**：150ms/步
- **局部刷新**：只重绘蛇头和蛇尾
- **碰撞检测**：撞墙或撞自己游戏结束

#### 2. 推箱子游戏
- **地图大小**：10×8
- **关卡数量**：3关
- **局部刷新**：只重绘玩家和箱子移动的格子
- **胜利条件**：所有箱子推到目标点

#### 3. 俄罗斯方块
- **游戏区域**：10×20
- **方块类型**：7种标准方块 (I, O, T, S, Z, J, L)
- **下落速度**：500ms/步
- **局部刷新**：记录旧位置和形状，只重绘变化区域
- **消行计分**：每行100分

#### 4. 打砖块游戏
- **砖块阵列**：8列×5行
- **局部刷新**：只重绘球和挡板移动区域
- **碰撞检测**：球与砖块、挡板、墙壁的碰撞

### 局部刷新优化

所有游戏都实现了局部刷新机制，避免全屏重绘导致的闪烁：

```cpp
// 俄罗斯方块示例
void drawTetrisGame() {
  if (tetrisFirstDraw) {
    // 首次绘制：全屏刷新
    display.fillScreen(TFT_BLACK);
    drawAllFixedPieces();
    tetrisFirstDraw = false;
  } else {
    // 局部刷新：只清除旧位置
    clearOldPiece(lastPieceX, lastPieceY, lastPieceShape);
    // 绘制新位置
    drawCurrentPiece(currentPieceX, currentPieceY, currentPiece);
  }
  // 更新记录
  lastPieceX = currentPieceX;
  lastPieceY = currentPieceY;
  copyPiece(lastPieceShape, currentPiece);
}
```

### 输入处理

#### 摇杆控制
```cpp
bool readJoystick(int& dirX, int& dirY, bool& press) {
  // 防抖处理 (150ms)
  if (millis() - lastDebounceTime < debounceDelay) return false;
  
  // 检测5个方向
  if (digitalRead(PIN_UP) == LOW) { dirX=0; dirY=-1; ... }
  // ... 其他方向
}
```

#### 顶部按键
```cpp
TopKey readTopButtons() {
  // 防抖处理 (200ms)
  // A: 暂停/继续
  // B: 重新开始
  // C: 返回菜单
}
```

---

## 🔍 问题排查记录

### 已知问题及解决方案

#### 1. 俄罗斯方块闪烁问题
**现象**：方块下落时闪烁严重，移动路径有残影

**根因**：
- 每帧都调用 `drawTetrisGame()`，即使方块未移动
- 旋转后 `currentPiece` 已改变，但清除旧方块时用了新形状
- 方块锁定后，旧位置记录未清除，导致固定方块被误删

**解决方案**：
- 添加 `lastPieceShape[4][4]` 记录旧形状
- 只在方块移动/旋转/下落时重绘
- 方块锁定后设置 `tetrisFirstDraw = true` 触发全屏重绘

#### 2. 推箱子第二关无解
**现象**：箱子被推到角落无法移动

**根因**：初始地图设计不合理，箱子位置导致死锁

**解决方案**：调整箱子位置，确保关卡可解

#### 3. 推箱子墙壁闪烁
**现象**：每次移动都重绘整个地图，墙壁闪烁

**解决方案**：实现局部刷新，只重绘玩家和箱子移动的格子

#### 4. 俄罗斯方块分数显示不全
**现象**：分数在屏幕底部被截断

**解决方案**：将分数移至屏幕右侧 (X=260, Y=60)

### 编译/上传问题

| 问题 | 解决方案 |
|------|----------|
| 找不到 Seeed_GFX 库 | 通过库管理器安装 Seeed Arduino GFX |
| 上传失败 | 检查 USB 连接，确保选择正确的串口 |
| 屏幕不显示 | 确认 WIO_TERMINAL_PRODUCT 初始化正确 |

---

## 📁 项目文件结构

```
sketch_aug10a/
├── sketch_aug10a.ino          # 主程序文件
├── README.md                  # 英文版本文档
├── README.zh-CN.md            # 本文件(简体中文)
└── PROJECT_DOCUMENTATION.md   # 原中文技术文档
```

## 🚀 快速开始

1. **连接设备**：USB-C 连接 Wio Terminal 到电脑
2. **打开项目**：在 Arduino IDE 中打开 `sketch_aug10a.ino`
3. **选择开发板**：工具 → 开发板 → Seeed Wio Terminal
4. **上传代码**：点击上传按钮
5. **开始游戏**：设备重启后显示主菜单，使用摇杆选择游戏

## 🎮 操作说明

| 操作 | 控制 |
|------|------|
| 菜单选择 | 摇杆上/下 |
| 确认选择 | 摇杆按下 |
| 游戏控制 | 摇杆方向 |
| 暂停/继续 | 顶部按键 A |
| 重新开始 | 顶部按键 B |
| 返回菜单 | 顶部按键 C |

---

## 📝 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2026-08-12 | 初始版本，包含4款游戏和传感器工具 |

## 📄 许可证

本项目基于开源代码开发，可自由修改和分发。