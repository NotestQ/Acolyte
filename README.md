# Acolyte
Acolyte is a mod loader and SDK for Vagante, providing a (currently) C++ API to integrate with the proprietary engine. Its goal is to make the game at least partly moddable.

Vagante ships with its debug symbols (.pdb), which make up roughly 40% of the game’s total size. This makes it unusually accessible for reverse engineering compared to most paid games.

Given that, it is *obviously* a good starting point for ~~masochists~~ experienced programmers new to reverse engineering, hooking, and C++

## Manual Installation
> [!CAUTION]
> Native mods can execute arbitrary code. Acolyte currently does not sandbox or validate mods.
> 
> Malicious or buggy mods can **corrupt your save or compromise your system**.
> 
> Back up your original savedata.sav in multiple places.

1. Download the [latest release](https://github.com/NotestQ/Acolyte/releases), version.dll
2. Move version.dll into the game directory. You can navigate to the game's directory by right-clicking the game in Steam, hovering "Manage", and selecting "Browse local files".
3. Launch the game once to generate the Acolyte/ directory.
4. Put mod directories in Acolyte/mods
5. Run Vagante.

## Project Status
Acolyte is experimental and under active but irregular development.
APIs are unstable. Breaking changes should be expected.

**Do not use on saves you care about or ones not backed up.**

### Scope
Acolyte provides:
- A native mod loader for Vagante
- A C++ SDK for injecting and extending game behavior
- Low-level access via hooks and reverse-engineered symbols
- API for player classes (partly)

Does not aim to provide:
- Official mod support parity
- Cross-platform support (Windows only)
  
Plans to provide:
- Full player class API
- Stable loader core, logging, configs, warnings, etc...
- Safer save handling
- Documentation and examples
- Luau scripting support, meaning a sandboxed high-level modding API
  
Aspires to provide:
- Custom Items
- Custom Weapons
- Custom Affinities
- Custom Backgrounds
- Custom Rooms
- Custom Enemies
- Additional game content and systems

## Developing
You will need:
- Windows
- Visual Studio 2022+ is recommended, a compiler that accepts C++23 is the minimum
- Basic knowledge of C++. And for anything more complicated than the SDK provides, familiarity with memory layout and reverse engineering

### Mod Structure
Acolyte loads every .dll in a mod directory. A manifest.toml is required for metadata and versioning.

```
Acolyte/
└─ mods/
   └─ ExampleMod/
      ├─ ExampleMod.dll
      ├─ manifest.toml
      └─ etc...
```
```toml
# manifest.toml
name = "ExampleMod"
version = "0.0.0" # Optional *for now*, but console will scream about it otherwise
```

## Extras

### Why Acolyte
The name comes from Vagante’s backgrounds (Acolyte of Light, Acolyte of Chaos, etc...), reflecting the tool’s purpose to assist the community in modding Vagante.

### Credits
- Notest – Lead developer  
- Vagante team – For the game and debug symbols that made this possible
  
**Special thanks** to the following projects and teams for tools, libraries, and inspiration:
- [Ghidra](https://github.com/NationalSecurityAgency/ghidra)
- [x64dbg](https://github.com/x64dbg/x64dbg)
- [Cheat Engine](https://github.com/cheat-engine/cheat-engine)
- [Safetyhook](https://github.com/cursey/safetyhook/)
- [MinHook](https://github.com/TsudaKageyu/minhook)
- [perfect dll proxy](https://github.com/mrexodia/perfect-dll-proxy)
- [colors.hpp](https://github.com/hugorplobo/colors.hpp)
- [toml++](https://github.com/marzer/tomlplusplus)
- [sigslot](https://github.com/palacaze/sigslot)
