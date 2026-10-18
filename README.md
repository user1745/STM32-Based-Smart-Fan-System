<div align="center">

# STM32-Based Smart Fan System

**基于 STM32 的智能风扇系统 —— 集成 OLED 菜单、手动/自动/定时/蓝牙控制、温湿度与烟雾检测。**

[![Platform](https://img.shields.io/badge/Platform-STM32-informational.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-microcontrollers.html)
[![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f1-series.html)
[![Toolchain](https://img.shields.io/badge/Toolchain-Keil%20MDK-blue.svg)](https://www.keil.com/)

</div>

## 目录

- [项目简介](#项目简介)
- [演示与硬件](#演示与硬件)
- [系统功能](#系统功能)
- [烧录与构建](#烧录与构建)
- [工程结构](#工程结构)
- [安全与调试注意事项](#安全与调试注意事项)
- [许可证](#许可证)

## 项目简介

本项目是一套基于 `STM32F103C8T6` 的智能风扇控制系统，使用 OLED 菜单和旋转编码器进行本地交互，并结合传感器数据实现自动控温、烟雾报警、排烟和定时运行。

系统同时提供 HC-05 蓝牙控制入口，可通过手机端“蓝牙调试器”远程切换运行模式。

## 演示与硬件

- [观看「基于 STM32 的智能风扇系统」演示视频](https://www.bilibili.com/video/BV1SdLn6JES4?vd_source=8a3cfebb5b0c7324aa168aa85736dd91)
- [PCB 开源连接：智能风扇系统板](https://oshwhub.com/dongjianxue/zhi-neng-feng-shan-xi-tong-ban)

### 主要硬件

| 模块 | 用途 |
| --- | --- |
| STM32F103C8T6 最小系统板 | 主控 |
| TB6612 电机驱动模块 | 电机方向与速度控制 |
| MP1584EN 降压模块 | 电源转换 |
| 0.96 寸 OLED | 菜单和状态显示 |
| HC-05 蓝牙模块 | 手机远程控制与串口下载 |
| Encoder 旋转编码器 | 菜单导航和参数调整 |
| BEEP 蜂鸣器 | 声音提示与报警 |
| DHT11 | 温湿度采集 |
| MQ-2 | 烟雾检测 |
| SG90 舵机 | 风扇摇头 |
| 5V 小电机与风扇叶 | 风扇执行机构 |
| 12V 电池及连接件 | 供电与装配 |

## 系统功能

### 智能菜单

OLED 显示多级菜单，使用旋转编码器和按键快速导航、修改参数并确认运行模式。

### 四种控制模式

| 模式 | 功能 |
| --- | --- |
| 手动模式 | 设置旋转方向、速度和是否开启风扇摇头 |
| 自动模式 | 根据温湿度自动开关和调节速度；检测到烟雾时声光报警并快速反转排烟 |
| 定时模式 | 设置运行参数和关闭倒计时，倒计时结束后自动停止 |
| 蓝牙模式 | 通过手机蓝牙调试器远程切换并控制上述模式 |

自动模式中的温湿度阈值和烟雾阈值可通过旋转编码器调整。

## 烧录与构建

### ST-Link 下载

可以使用 ST-Link 对程序进行常规编译和下载，具体目标配置以 Keil 工程为准。

### HC-05 蓝牙串口下载

蓝牙串口下载使用 Microsoft Store 中的 `MCUIAP`。若通过 HC-05 下载程序，需要同步检查以下配置：

1. 在 Keil 中开启调试信息并生成 `.hex` 文件。
2. 将代码运行起始地址改为 `0x08005000`，偏移地址改为 `0xB000`。
3. 在 `system_stm32f10x.c` 中将 `VECT_TAB_OFFSET` 改为 `0x5000`。
4. 在 `Usart_Init()` 中将串口波特率改为 `460800`。
5. 在 `Uart.c` 中保留或实现连续接收 5 个 `0xAA` 后重启的 IAP 握手逻辑。
6. 在 `main.c` 中解除中断屏蔽并初始化串口：

   ```c
   __set_PRIMASK(0);
   Usart_Init();
   ```

> 以上参数依赖当前工程的 IAP/启动配置；修改前请备份原工程，并确认 Bootloader 与应用程序地址一致。

## 工程结构

```text
├── Control/              # 控制逻辑
├── Hardware/             # 外设和传感器驱动
├── Library/              # 库文件
├── Start/                # 启动文件
├── System/               # 系统配置
├── User/                 # 用户应用代码
├── Objects/              # 编译输出
├── Project.uvprojx       # Keil 工程文件
├── Project.uvoptx        # Keil 配置文件
├── README.assets/        # 文档资源
└── README.md
```

## 安全与调试注意事项

- 电机、12V 电池和降压模块存在机械及电气风险，首次上电请使用限流电源并空载测试。
- MQ-2、DHT11 和电机驱动模块的实际阈值需要结合模块批次和现场环境校准。
- 烟雾排放、喷气或反转动作不得替代消防安全措施；本项目仅用于学习和原型验证。
- 烧录地址、串口波特率和 Bootloader 逻辑必须成套匹配，否则可能无法启动或覆盖程序。
- 修改工程配置前请保留可恢复的 `.hex` 和 Keil 工程副本。

## 许可证

本仓库当前未声明开源许可证。代码、硬件资料和文档的使用、修改与再分发请先获得作者授权；PCB 资料的授权范围以立创开源平台页面说明为准。