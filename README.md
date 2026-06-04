# Nemesis

A 6v6 competitive arena shooter built on the **NovaCore Engine**. Fast-paced gameplay with advanced movement mechanics, precise gunplay, and skill-based combat.

## Overview

Nemesis is a ground-up game project targeting a commercial-ready competitive FPS experience with:

- 6v6 team-based gameplay
- Advanced movement system (Apex-like with BO-like TTK)
- Precise gunplay and weapon balance
- Multiple game modes
- Custom networking architecture
- Cross-platform support (Windows/Linux first)

## Project Structure

- `game/` - Game logic and main executable
- `configs/` - Game configuration files (modes, weapons, movement, input)
- `assets/` - Game assets (models, textures, animations)
- `docs/` - Game design and implementation documentation

## Build Requirements

- NovaCore Engine (separate repository)
- C++23 compatible compiler
- CMake 3.27+
- Ninja

## Build Instructions

The game requires the NovaCore Engine to be available in your build environment.

### With full support:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

### Without external dependencies:

```powershell
cmake --preset local-debug-no-deps
cmake --build --preset local-debug-no-deps
```

## Documentation

See `docs/` for game design, mechanics, and implementation guides.

## Related Projects

- **NovaCore Engine**: https://github.com/YoungJibbit95/Novacore-Engine

## License

See LICENSE file for details.
