<h1 align="center">Lumen</h1>
<div align="center">

<a href="https://github.com/robcholz/Lumen/actions"><img src="https://img.shields.io/github/actions/workflow/status/robcholz/Lumen/build.yml?label=CI&branch=main"/>
<a href="https://github.com/robcholz/Lumen"><img src="https://img.shields.io/badge/hardware-open--source-brightgreen"/>
<a href="https://github.com/robcholz/Lumen/blob/main/LICENSE"><img src="https://img.shields.io/github/license/robcholz/Lumen?color=2b9348" alt="License Badge"/></a>

<p align="center">
     <a href="README.md">English</a> | <a href="README-zh-CN.md">简体中文</a>
  </p>

<i>喜欢这个项目吗？请考虑给 Star ⭐️ 以帮助改进！</i>

</div>

---

> 一个把电脑、游戏和系统状态在你的桌面上展示的硬件设备。
>
> 可以把它看作是 Stream Deck，
> 但可以在固件层级编程，并且完全开源。

![](docs/banner.gif)

> **🔥火速复刻 Lumen。**
>
> 不需要本地 IDE，不需要配置工具链，也不需要懂贴片焊接。
> 即使你对嵌入式和硬件并不熟悉，也可以完成。
>
> 👉 从这里开始复刻： [快速开始](docs/quick-start-zh-CN.md)

## Lumen 是什么？

Lumen 是一个桌面级硬件交互节点，
它长期存在于桌面环境中，能够感知状态、参与交互，
并将这些变化以硬件方式呈现和反馈出来。

## 为什么选择 Lumen？

Lumen被设计为可以每天放在桌面上使用，
并通过配置来适应不同的使用场景。

如果你喜欢把虚拟世界连接至现实世界的项目，
Lumen 是为你量身打造的。

## 10 分钟内做出你的 Lumen

- 直接用浏览器烧录固件
- 不需要本地 IDE 或工具链
- 所有构建由 CI 自动生成
- 硬件文件完全开源

👉 从这里开始复刻： [快速开始](docs/quick-start-zh-CN.md)

## 大家用 Lumen 做什么

- 一个实体化的 Minecraft 状态显示器
- 一个常驻桌面的 USB-C 电源监控
- 一个系统状态提示器（CPU、构建、部署、错误）
- 面向游戏和工具的可编程交互装置

Lumen 不是单一用途设备，它是一个可以被你重新定义的硬件平台。

## 实际效果

**Minecraft → 实体设备**

![](docs/mc_sync.gif)

当 Minecraft 里发生事情时，
Lumen会实时响应 — 灯光、动作与声音。

## 实物与界面预览

**USB 电源监控**

![](docs/usb.gif)

**动作 / 交互**

![](docs/motion.gif)

**渲染图**

![](docs/preview/view5.png)

![](docs/preview/view1.png)

## 系统架构概览

- 系统集成: Rust no-std
- UI：Vision-UI
- 系统：FreeRTOS
- Graphics Driver：u8g2
- 硬件
    - 蜂鸣器
    - INA226
    - LSM6DSO
    - ESP32-C3
    - 240x240显示屏
    - 转轮编码器
    - Power Switch

## 设计笔记

### 关于架构

- 固件采用分层Driver结构，而不是功能堆叠，目的是加速开发和CI构建。
- UI 独立于业务逻辑，确保交互修改不会影响核心功能
- Rust no-std 仅用于系统集成层，而不是全面替代 C/C++

### 关于交互

- 所有交互优先考虑「长期桌面使用」，而不是调试便利性
- UI 不依赖串口输出，避免设备在“脱离电脑”后失去价值
- 动效与彩蛋被刻意控制在低频范围内，避免干扰主功能

### 关于可复刻性

- 所有构建流程自动化，避免“作者环境依赖”
- 避免需要昂贵设备或特殊工具的方案
- 明确不追求极限性能，而优先保证成功率

### 已知限制

- Web 烧录流程依赖浏览器对 WebUSB 的支持
- CI没有cache和diff构建，仍有速度优化空间

## 延伸阅读

如果你对 Lumen 所使用的 UI 系统感兴趣，可以参考这篇文章：

- [Vision-UI：一个面向嵌入式设备的轻量级 UI 系统设计](https://www.robcholz.com/2025/11/16/vision-ui/)

## 项目状态

- [x] 硬件v2
- [x] USB-C电源监控
- [x] 硬件保护逻辑
- [x] Vision-UI集成
- [x] CI流水线
- [x] Web快速烧录
- [ ] 文档持续完善中

## 参与方式

如果这个项目对你有帮助， 可以给它点个⭐，这会帮助我们继续开发Lumen！

欢迎 Issue、讨论和 PR。
如果你对硬件、嵌入式系统或 UI 设计感兴趣，都可以参与进来。

## License

GPL-3.0
