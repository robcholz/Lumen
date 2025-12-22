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

> 一个专注于交互细节的桌面级硬件项目，
> 用于 USB-C 电源监控与保护。
>
> 从硬件到固件开源，
> 在严肃功能之外，保留了一点趣味性。

![](docs/banner.gif)

> **🔥火速复刻 Lumen。**
>
> 不需要本地 IDE，不需要配置工具链，也不需要懂贴片焊接。
> 即使你对嵌入式和硬件并不熟悉，也可以完成。
>
> 👉 从这里开始复刻： [快速开始](docs/quick-start-zh-CN.md)

## Lumen 是什么？

Lumen 是一个桌面级 USB-C 电源监控与保护设备，
面向开发者，同时也对非专业用户友好。

我们希望它不仅仅是一个调试工具，
而是一个可以被长期放在桌面上使用的设备。

## 为什么做 Lumen？

很多桌面电源监控相关的开源项目，存在这些问题：

- 只提供代码，缺少完整硬件资料，无法真正复刻
- 需要复杂的 IDE 与工具链，新手几乎不可能成功
- UI 仅停留在串口打印或调试界面，难以长期使用

Lumen 的目标很明确：

让没有嵌入式或硬件背景的人，
也能成功做出一个「真的能用、而且好用」的 USB-C 电源监控设备。

为此，Lumen 从一开始就围绕「可复刻性」和「低门槛」进行设计，
尽量把复杂性留在系统内部，而不是留给使用者。

## 为「可复刻性」而设计

- 无需本地开发环境，直接通过网页完成固件烧录
- 所有固件由 CI 自动构建与发布，避免环境差异
- 硬件、固件与 UI 设计集中在同一个仓库中
- PCB Gerber、BOM、3D 外壳文件全部开源

**你只需要照着步骤做就能完整复刻。**

## 实物与界面预览

**USB 电源监控**

![](docs/usb.gif)

**Minecraft彩蛋**

![](docs/banner.gif)

**Motion / 交互**

![](docs/motion.gif)

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

欢迎 Issue、讨论和 PR。
如果你对硬件、嵌入式系统或 UI 设计感兴趣，都可以参与进来。

## License

GPL-3.0