<h1 align="center">Lumen</h1>
<div align="center">

<a href="https://github.com/robcholz/Lumen/actions"><img src="https://img.shields.io/github/actions/workflow/status/robcholz/Lumen/build.yml?label=CI&branch=main"/>
<a href="https://github.com/robcholz/Lumen"><img src="https://img.shields.io/badge/hardware-open--source-brightgreen"/>
<a href="https://github.com/robcholz/Lumen/blob/main/LICENSE"><img src="https://img.shields.io/github/license/robcholz/Lumen?color=2b9348" alt="License Badge"/></a>

<p align="center">
     <a href="README.md">English</a> | <a href="README-zh-CN.md">简体中文</a>
  </p>

<i>Like this project? Please consider giving it a Star ⭐️ to help it grow!</i>

</div>

---

> A hardware interaction node that lives on the desktop long-term,
> built to carry sensing, feedback, and interaction logic.
>
> Open-sourced from hardware to firmware,
> with a little fun beyond the serious functionality.

![](docs/banner.gif)

> **🔥 Build Lumen FAST on your own.**
>
> No local IDE, no toolchain setup, no need to know SMT soldering.
> Even if you are not familiar with embedded systems or hardware, you can finish it.
>
> 👉 Start here: [Quick Start](docs/quick-start.md)

## What is Lumen?

Lumen is a desktop-grade hardware interaction node.
It lives on the desktop long-term, can sense state, participate in interaction,
and present and feed back those changes in hardware.

## Why we build Lumen?

Many desktop hardware projects start from a very specific use.
At first, they seem quite useful, but as soon as you want to do a bit more, something feels off.

I found that when people keep this kind of device on their desk long-term, problems slowly surface:

- Projects only focus on function validation, lack complete materials, and are hard to truly reproduce
- Depend on complex IDEs and toolchains, with a high barrier to use
- Interaction stays at serial or debug interfaces, not suitable for long-term use

These are tolerable while tinkering,
but once it becomes something that is there every day, it feels awful.

Lumen also started from a specific need.
But as you work on it, you realize it should not stop at a single function.
A more reasonable state is to keep it on the desk long-term,
as a hardware interaction node that can continuously expand.

**We hope this project can let people without embedded or hardware backgrounds
successfully build a device that is "truly usable for the long term";
and on top of that, it can sense states and participate in interaction,
serving as a connection point between the system, software or games, and the real world.**

For this, Lumen is designed from the beginning around
"reproducibility" and "low barrier of use",
keeping complexity inside the system rather than pushing it to the user.

What you end up with is not a single function,
but a hardware presence that can stay on the desk long-term and evolve with needs.

## Designed for "Reproducibility"

- No local development environment needed, flash firmware directly via web
- All firmware is built and released by CI, avoiding environment differences
- Hardware, firmware, and UI design are centralized in one repo
- PCB Gerbers, BOM, and 3D enclosure files are fully open-sourced

**You only need to follow the steps to fully reproduce the entire device.**

## Hardware and UI Preview

**USB power monitoring**

![](docs/usb.gif)

**Interact with the game "Minecraft"**

![](docs/mc_sync.gif)

**Motion / interaction**

![](docs/motion.gif)

![](docs/preview/view5.png)

![](docs/preview/view1.png)

## System Architecture Overview

- System integration: Rust no-std
- UI: Vision-UI
- System: FreeRTOS
- Graphics Driver: u8g2
- Hardware
    - Buzzer
    - INA226
    - LSM6DSO
    - ESP32-C3
    - 240x240 display
    - Rotary encoder
    - Power Switch

## Design Notes

### About architecture

- Firmware uses a layered driver structure instead of feature stacking, to speed up development and CI builds.
- UI is independent of top level logic, ensuring interaction changes do not affect core functionality.
- Rust no-std is used only for the system integration layer, not to fully replace C/C++.

### About interaction

- All interactions prioritize "long-term desktop use" rather than debugging convenience.
- UI does not depend on serial output, so the device does not lose value when "detached from the PC".
- Animations and easter eggs are deliberately kept low-frequency to avoid distracting the main function.

### About reproducibility

- All build flows are automated to avoid "author environment" dependency.
- Avoids solutions that require expensive equipment or special tools.
- Explicitly not chasing extreme performance, prioritizing success rate.

### Known limitations

- The web flashing flow depends on browser WebUSB support.
- CI has no cache or diff builds yet, so there is room for speed optimization.

## Further Reading

If you are interested in the UI system used in Lumen,
you may find this article useful:

- [Vision-UI: Designing a Lightweight UI System for Embedded Devices](https://www.robcholz.com/2025/11/16/vision-ui/)

## Project Status

- [x] Hardware v2
- [x] USB-C power monitoring
- [x] Hardware protection logic
- [x] Vision-UI integration
- [x] CI pipeline
- [x] Web quick flashing
- [ ] Documentation still improving

## Contributing

Issues, discussions, and PRs are welcome.
If you are interested in hardware, embedded systems, or UI design, feel free to join.

## License

GPL-3.0
