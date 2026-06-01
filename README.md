# maxfetch – Maximum System Info Tool

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Linux](https://img.shields.io/badge/platform-Linux-green.svg)](https://www.linux.org/)
[![Arch](https://img.shields.io/badge/Arch-Linux-1793D1.svg)](https://archlinux.org/)
[![Debian](https://img.shields.io/badge/Debian-D70A53.svg)](https://www.debian.org/)
[![Fedora](https://img.shields.io/badge/Fedora-294172.svg)](https://getfedora.org/)

**maxfetch** is a powerful, fast, and colorful system information tool for Linux.  
It displays everything you need to know about your PC – CPU, GPU, memory, disks, network, USB/PCI devices, temperatures, battery, and much more – in an adaptive multi‑column layout.  
No logo, no bloat – just **maximum information** at a glance.

**Repository:** [github.com/proximacentav/maxfetch](https://github.com/proximacentav/maxfetch)

---

## ✨ Features

- **Comprehensive hardware info** – CPU (model, cores, frequency, cache), RAM + swap, GPU (model + driver), disks (model, size, usage), motherboard, BIOS.
-  **Sensors** – CPU/thermal zone temperatures (via `/sys/class/thermal` or `sensors`).
-  **Battery status** (capacity, charging state).
-  **Network details** – interfaces (MAC, speed), IPv4/IPv6 addresses, active Wi‑Fi SSID, local ARP/neighbour table.
-  **Peripherals** – USB devices, PCI devices (network, audio, storage), audio cards, screen resolutions.
-  **Disk usage** – from `lsblk` or fallback to `df`.
-  **Colour coding** – system info (cyan), hardware (green), peripherals & network (yellow).
-  **Adaptive columns** – automatically switches between 1 and 6 columns based on terminal width.
-  **Blazing fast** – reads most data from `/proc`, `/sys`, and minimal external commands.

---

## Installation

### Option 0: From source (compile yourself)

```bash
git clone https://github.com/proximacentav/maxfetch.git
cd maxfetch
g++ -std=c++17 -O2 -o maxfetch maxfetch.cpp
sudo cp maxfetch /usr/local/bin/
