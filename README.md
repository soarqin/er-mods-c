# er-mods-c (YAERModLoader)

A collection of quality-of-life mods for the PC (Steam) version of *ELDEN RING*, written in C.
Targets game patch 1.12 or later, including *Shadow of the Erdtree*.

Each mod is built as a standalone DLL. Load the DLLs into the game with your preferred DLL loader/injector.

> These mods are intended for offline / single-player use only (the game must be launched without Easy Anti-Cheat). Use at your own risk.

## Mods

Mods with configurable options read their `<mod_name>.ini` from the same directory as the DLL. `almighty_kale` and `itemlot_rate` require `er_param.dll` to be loaded as well.

### almighty_kale

Makes Merchant Kalé glorious: talk to Kalé to freely buy and sell almost every item in the game — weapons, armor, talismans, sorceries, incantations, ashes of war, and goods. This is a 1:1 C port of the [Glorious Merchant](https://github.com/ThomasJClark/elden-ring-glorious-merchant) mod.

Options (`almighty_kale.ini`):

| Option | Default | Description |
| --- | --- | --- |
| `auto_upgrade_weapons` | `true` | Weapons sold by the shop come upgraded to the highest upgrade level the current character has ever achieved. Set to `false` to sell all weapons at +0. |

### autoloot

Automatically picks up items and lost runes lying on the ground — no need to press the interact button.

### er_param

Core infrastructure DLL: reads the game's param files and exposes them to other mods (`almighty_kale`, `itemlot_rate`). It also includes a small tweak of its own.

Options (`er_param.ini`):

| Option | Default | Description |
| --- | --- | --- |
| `world_map_cursor_speed` | `1.0` | Movement speed multiplier for the world map cursor. Clamped between 0.5 and 10. `1` = game default. |

### itemlot_rate

Improves item drop rates: makes selected equipment always drop from enemies, and lets you change the drop quantity of specific items.

Options (`itemlot_rate.ini`):

| Option | Default | Description |
| --- | --- | --- |
| `include_weapons` | `true` | All weapons drop at 100% rate. |
| `include_arrows` | `false` | Arrows and other ammunition drop at 100% rate. |
| `include_armors` | `true` | All armor pieces drop at 100% rate. |
| `include_goods` | *(empty)* | Comma-separated list of item IDs; these goods always drop, e.g. `include_goods=15080,15140`. |

The `[alter_count]` section changes the drop quantity of specific items:

```ini
[alter_count]
; Format: item_id = type, count
15080=drop,10
20811=loot,10
```

- `type` — where the entry applies:
  - `drop` — items dropped by enemies
  - `loot` — items picked up from shiny points on the ground
  - `all` — both of the above
- `count` — the quantity granted per drop.

`itemlot_rate.zhCN.ini` is a Simplified-Chinese-commented copy of the same file kept for reference; the mod only reads `itemlot_rate.ini`.

### no_dup_loot

Prevents enemies from dropping equipment and items you already own, so repeat runs don't clutter the ground with duplicates.

### remove_ca_vig

Disables the chromatic aberration and vignette post-processing effects.

### remove_menu_input_delay

Removes the extra input delay before confirm inputs register in conversation menus and yes/no dialogs, introduced by patch 1.12 — restoring the pre-1.12 responsiveness.

### reset_achievements_on_new_game

Automatically resets all Steam achievements and stats when you start a new game, so achievements can be earned again from scratch on a fresh playthrough.

## Building

Prerequisites:

- Windows
- [CMake](https://cmake.org) 3.15 or later
- Visual Studio 2022 with the "Desktop development with C++" workload (MSVC)
- [NASM](https://www.nasm.us) available on `PATH` (required by `autoloot`)

```bat
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Each mod is output as its own DLL under `build\bin\dll\`, e.g. `almighty_kale.dll`, `er_param.dll`. For a debug build, use `--config Debug`.

## Credits

- [ThomasJClark](https://github.com/ThomasJClark) — [Glorious Merchant](https://github.com/ThomasJClark/elden-ring-glorious-merchant) (MIT), which `almighty_kale` is a 1:1 C port of.
- [tremwil](https://github.com/tremwil) — for the suggestion and help with the assembly hook wrapper used by `autoloot`.
- Third-party libraries vendored under [`deps/`](deps/):
  - [inih](https://github.com/benhoyt/inih) — INI file parser by Ben Hoyt (New BSD license)
  - [klib](https://github.com/attractivechaos/klib) — `khash` hash map by Attractive Chaos (MIT license)
  - [MinHook](https://github.com/TsudaKageyu/minhook) — x64/x86 API hooking library by Tsuda Kageyu (BSD 2-clause license)
- Parameter structure definitions are generated from the community-maintained [Paramdex](https://github.com/soulsmods/Paramdex) dataset.

## License

This project is licensed under the [MIT License](LICENSE) — Copyright (c) 2024 Soar Qin.

Vendored third-party libraries keep their own licenses (see `deps/*/LICENSE.txt`).
