# PKBankVC (Pokémon Bank for Virtual Console)
[![Platform](https://img.shields.io/badge/Platform-Nintendo%203DS-red.svg)](https://www.nintendo.com/3ds/)
[![Formats](https://img.shields.io/badge/Format-3DSX%20%7C%20CIA-blue.svg)]()
[![License](https://img.shields.io/badge/License-GPLv3-green.svg)](LICENSE)
[![Author](https://img.shields.io/badge/Author-SilverDemon-orange.svg)]()

**PKBankVC** is an all-in-one, homebrew Pokémon Bank and Save Manager built specifically for the Game Boy (Gen I) and Game Boy Color (Gen II) Virtual Console titles on the Nintendo 3DS family of systems.

Featuring a dedicated 100-box storage cloud (up to 3,200 Pokémon), an authentic animated Pokédex, full Bag & PC Item storage with multi-generation pocket filters, and a touch-screen Pokémon Editor with shiny calculation, IV/EV manipulation, and quick drag-and-drop cloning.

---

## 🌟 Key Features

### 🏦 100-Box Bank Storage (3,200 Pokémon)
- Store up to **3,200 Pokémon** across 100 secure boxes saved directly to `/pk/bank/bankgb.bin` on your SD card.
- Seamlessly transfer Pokémon between Gen I and Gen II games with automatic validation and type/stat conversion.
- Full compatibility with eggs (Gen II), shininess, original trainer IDs, nicknames, and experience.

### 🎮 Comprehensive Virtual Console Compatibility
Supports all 7 main-line Game Boy and Game Boy Color releases across every language and region:
- **Generation I:** Pokémon Red, Green, Blue, and Yellow (EN / ES / FR / DE / IT / JP).
- **Generation II:** Pokémon Gold, Silver, and Crystal (EN / ES / FR / DE / IT / JP).
- Automatic save discovery via Virtual Console title detection (`Title Selector`) with live trainer preview card.
- Automatic SRAM bank detection and checksum re-calculation (including Crystal backup mirrored checksums).

### 📖 Authentic Pokédex & Habitat Map
- Authentic Pokémon Crystal UI with full 56x56 animated Crystal front sprites.
- **Dynamic Discovery Mode:** Pokémon remain as `?????` (Unknown) until deposited into the Bank.
- **Interactive World Map:** Visual habitat tracker for Kanto and Johto with flashing species indicators and route names.
- **Official Diploma Verification:** Celadon Game Freak Diplomas (150 Kanto / 250 Johto) verified directly against your active game save data.

### 🎒 Multi-Generation Bag & PC Item Storage
- Dedicated 100-slot Bank Item Storage for holding items, TMs, HMs, and Key Items.
- Smart item conversion between Gen I and Gen II item IDs.
- Automatic security locks preventing Gen II items or held items from entering Gen I games.

### ✨ Built-in Touch Pokémon Editor
Press **START** on any Pokémon to access the touch editor:
- **Instant Shiny Converter:** Automatically generates valid Gen II shiny IV spreads ($10/10/10/10$).
- **DV / IV & EV Customizer:** Full manual control over Attack, Defense, Speed, and Special stats.
- **Level & Nickname Editor:** Easily adjust levels or fix nicknames on the fly.
- **Touch-and-Hold Quick Clone:** Hold any Pokémon on the bottom screen for 2.5 seconds to clone and place it anywhere in your boxes.

---

## 🕹️ Controls

| Button | Action |
| :--- | :--- |
| **D-Pad / Circle Pad** | Navigate boxes and Pokémon slots |
| **A** | Select / Pick up Pokémon / Open Diploma (in Dex) |
| **B** | Cancel selection / Go back |
| **X** | Open Bag & Item Storage |
| **Y** | Open Bank Pokédex & Habitat Map |
| **START** | Open Pokémon Editor / Save & Exit Editor |
| **SELECT** | Return to Title Selector |
| **L / R** | Switch active box (Previous / Next) |
| **Stylus / Touch** | Direct slot selection, box switching & touch-and-hold cloning |

---

## 📦 Installation

### Method 1: CIA Installation (Recommended for Home Menu)
1. Copy `PKBankVC.cia` to your SD card (e.g., `/cias/`).
2. Open **FBI** on your 3DS.
3. Navigate to `SD` $\rightarrow$ `cias` $\rightarrow$ `PKBankVC.cia` and select **Install and delete CIA**.
4. Launch **PKBankVC** directly from your 3DS Home Menu.

### Method 2: Homebrew Launcher (.3DSX)
1. Copy `PKBankVC.3dsx` and `PKBankVC.smdh` to `sdmc:/3ds/PKBankVC/`.
2. Open **Homebrew Launcher** and launch **PKBankVC**.

---

## 🛠️ Building from Source

### Prerequisites
- [devkitPro](https://devkitpro.org/) with `devkitARM`.
- 3DS libraries: `libctru`, `citro2d`, `citro3d`, `tex3ds`.
- Python 3 (for build scripts).

### Compile
```bash
git clone https://github.com/SilverDemon/PKBankVC.git
cd PKBankVC
make
```
The compiled binaries (`PKBankVC.3dsx` and `PKBankVC.smdh`) will be generated in the root directory.

---

## 👥 Credits & Acknowledgments
- **SilverDemon** — Main developer, modern architecture, Pokédex, touch editor, item storage, and bug fixes.
- **Gocario** — Creator of the original PHBankGB project.
- **devkitPro & libctru contributors** — 3DS homebrew toolchain and graphics libraries.
- **Game Freak & Nintendo** — Pokémon series and Virtual Console releases.

---

## ⚖️ Disclaimer
*Pokémon is a registered trademark of Nintendo, Game Freak, and Creatures Inc. PKBankVC is a non-profit, fan-made open-source tool not affiliated with or endorsed by Nintendo or The Pokémon Company.*
