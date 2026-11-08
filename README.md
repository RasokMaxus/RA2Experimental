# RA2 Experimental

An experimental gameplay mod for **Command & Conquer: Yuri's Revenge** that changes how production works.

Classic C&C has one production queue per category: every War Factory you own shares a single vehicle queue, and extra factories only make that queue faster. This mod moves Yuri's Revenge towards the model of C&C Generals and C&C 3:

1. **Per-building production.** Click a production building and the sidebar shows what *that* building can build. Each building gets its own queue.
2. **Builder units** *(planned)*. A Generals-style dozer builds structures in the field, and the building rises as construction progresses.

It's also a learning project. The code is commented to explain *why* each engine hook exists, and [`MOD_DESIGN.md`](MOD_DESIGN.md) records the design decisions and reverse-engineering notes.

> **Status: early development.** Expect bugs. Play it in skirmish or LAN only: the modified engine won't match other CnCNet players online.

---

## Features

### Per-building production

| | Status |
|---|---|
| Sidebar shows only what the **selected** production building can build | ✅ Done (milestone 1) |
| Production tabs are **empty** when no production building is selected | ✅ Done |
| Selecting a factory **switches to its tab** (Construction Yard opens Structures) | ✅ Done |
| Production keeps running while its building is deselected; queues keep advancing | ✅ Done |
| Each building has its **own queue** (two War Factories = two queues) | 🚧 Next (milestone 2) |
| Units come out of the building that built them, with no "Primary" building | 🚧 Milestone 2 |
| Multiplayer-safe "produce at this building" network event | 🚧 Milestone 2 |
| AI uses its factories in parallel | 🚧 Milestone 2 |
| Airfields: planes that need a landing pad stay greyed out until that airfield has a free pad | 🚧 Milestone 2 |

Covered buildings: Construction Yard, Barracks, War Factory, Naval Yard and Airfield.

### Builder unit (planned)

- Construction Yards become "Command Centers" that train Dozers. The MCV model is a placeholder.
- A dozer places a building, drives there and constructs it. The **build-up animation plays in step with construction progress**.
- An unfinished building gives no power and unlocks nothing. It can be attacked, and its health grows as it is built.

---

## How it works

The mod is a fork of **[Phobos](https://github.com/Phobos-developers/Phobos)**, the open-source C++ engine extension for Yuri's Revenge. The original game is a 2001 executable with no source code. [SyringeEx](https://github.com/Phobos-developers/SyringeEx) starts the game and patches *hooks* into its machine code at known addresses. When the game reaches a hook, it jumps into our DLL, runs our code, and continues. [YRpp](https://github.com/Phobos-developers/YRpp) gives us C++ definitions of the game's own classes.

The mod's code is in [`src/Ext/Sidebar/PerBuildingProduction.cpp`](src/Ext/Sidebar/PerBuildingProduction.cpp). For a summary of each hook and why it's needed, see [`MOD_DESIGN.md`](MOD_DESIGN.md).

### Rules options

| Key | Section | Default | Meaning |
|---|---|---|---|
| `PerBuildingProduction` | `[General]` | `yes` | Turn the per-building sidebar on or off |

---

## Building from source

Requirements: Windows, Visual Studio 2022 or newer with **Desktop development with C++**, the **MSVC v143** build tools and **ATL**. You can import [`.vsconfig`](.vsconfig) in the Visual Studio Installer.

```bash
git clone --recurse-submodules https://github.com/RasokMaxus/RA2Experimental.git
```

Open `Phobos.sln` and build **Debug | x86**, or run `scripts\build_debug.bat`. The output is `Debug\Phobos.dll`.

## Installing for testing

1. Make a **separate copy** of a CnCNet Yuri's Revenge install that has Ares 3.0p1 and Phobos.
2. In the copy, disable the CnCNet updater by commenting out the URL in `updateconfig.ini`, so it can't overwrite the DLL.
3. Replace `Phobos.dll` (and `Phobos.pdb`) with your build.
4. Launch `CnCNetYRLauncher.exe` from the copy and start a skirmish.

**Ares is required.** Hidden cameos are removed silently through Phobos's redirect of Ares's cameo-removal function.

---

## Credits and license

This mod is built on the work of:

- **[Phobos](https://github.com/Phobos-developers/Phobos)** and its contributors (see [`CREDITS.md`](CREDITS.md)). The original Phobos README is kept as [`README.Phobos.md`](README.Phobos.md).
- **[Ares](https://github.com/Ares-Developers/Ares)**, **[YRpp](https://github.com/Phobos-developers/YRpp)** and **[SyringeEx](https://github.com/Phobos-developers/SyringeEx)**.
- **[CnCNet](https://cncnet.org)** for the client and spawner.

Like Phobos, this project is licensed under the **GNU GPL v3** (see [`LICENSE.md`](LICENSE.md)). If you distribute builds, the source must stay available.

*EA has not endorsed and does not support this product. Command & Conquer, Red Alert 2 and Yuri's Revenge are trademarks of Electronic Arts.*
