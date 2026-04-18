# muTerm

A fast, portable virtual terminal built for **MustardOS** handheld devices. Run a full shell, execute commands, and
navigate your device entirely from a tidy on-screen keyboard, or use a physical keyboard if so desired.

---

## What is muTerm?

muTerm is a terminal emulator that runs directly on your MustardOS device. It opens a real shell (bash, sh, or any program you choose), displays
its output on screen, and lets you type using a gamepad-driven on-screen keyboard (OSK). It supports colours, scrollback history, background images,
display rotation, and zoom. Everything you need for a proper terminal experience on a handheld.

It also works as a **read-only viewer**: pipe a programs output into it and watch it scroll in real time, with no input needed.

---

## Quick Start

### What you need

- The muTerm binary in your path, or called directly
- A monospace TTF font (see [Font Suggestions](#font-suggestions) below)

### Running muTerm

To open a specific program instead of a shell:

```
muterm -- htop
muterm -- bash -c "ls -la /opt"
```

### Basic gamepad controls

| Button           | Action                                                                           |
|------------------|----------------------------------------------------------------------------------|
| **Select**       | Cycle the on-screen keyboard (bottom → transparent → top → transparent → hidden) |
| **Menu / Guide** | Quit muTerm                                                                      |

**When the OSK is visible:**

| Button                   | Action                                                    |
|--------------------------|-----------------------------------------------------------|
| **D-pad / Left stick**   | Move the key cursor                                       |
| **A / Left stick click** | Press the highlighted key                                 |
| **B**                    | Backspace                                                 |
| **Y**                    | Space                                                     |
| **Start**                | Enter                                                     |
| **L1 / R1**              | Switch keyboard layer (lowercase → uppercase → Ctrl keys) |

**When the OSK is hidden:**

| Button              | Action                              |
|---------------------|-------------------------------------|
| **D-pad Up / Down** | Scroll through history              |
| **Vol+ / Vol−**     | Page up / page down through history |

---

## Configuration

muTerm reads its settings from config files, applied in this order (later files override earlier ones):

1. `/opt/muos/device/config` - device specific screen size and zoom (_specific for MustardOS_)
2. `/opt/muos/config` - global configuration settings (_specific for MustardOS_)
3. `/opt/muos/share/conf/muterm.conf` - system wide muTerm defaults
4. `~/.config/muterm/muterm.conf` - your personal overrides

A simple `muterm.conf` looks like this:

```ini
font_path = /opt/muos/share/font/muterm.ttf
font_size = 18
scrollback = 1024
bg_colour = 1a1a2e
fg_colour = e0e0ff
```

All the same options available on the command line (see below) can be placed in a config file
using `key = value` format, one per line. Lines starting with `#` are comments.

---

## Command Line Options

Every option can also be set in your config file. Command-line values always win.

### Display

| Option               | Example           | Description                                                                                                                |
|----------------------|-------------------|----------------------------------------------------------------------------------------------------------------------------|
| `-w`<br/>`--width <px>` | `--width 640`     | Window width in pixels. Defaults to the value from the MustardOS device config, or 640.                                    |
| `-h`<br/>`--height <px>` | `--height 480`    | Window height in pixels. Defaults to the value from the MustardOS device config, or 480.                                   |
| `--zoom <factor>`    | `--zoom 1.5`      | Scale the entire terminal up or down. `1.0` is normal size. `1.5` makes everything 50% larger. Useful on high-DPI screens. |
| `--rotate <0–3>`     | `--rotate 1`      | Rotate the display. `0` = normal, `1` = 90°, `2` = 180°, `3` = 270°. Matches MustardOS screen rotation settings.           |
| `--underscan`        | `--underscan`     | Add a 16-pixel border inset on all sides. Useful when connected over HDMI that crops the edges.                            |
| `--no-underscan`     | `--no-underscan`  | Force underscan off, even if the MustardOS global config enables it.                                                       |
| `-i`<br/>`--image <file>` | `--image /bg.png` | Use a PNG image as the terminal background instead of a solid colour. The image is stretched to fill the window.           |

### Font

| Option            | Example                                 | Description                                                                                    |
|-------------------|-----------------------------------------|------------------------------------------------------------------------------------------------|
| `-f`<br/>`--font <file>` | `--font /opt/muos/share/font/px437.ttf` | Path to a TrueType (`.ttf`) font file. Must be a **monospace** font for best experience.       |
| `-s`<br/>`--size <pt>` | `--size 16`                             | Font size in points. Larger values give bigger, more readable text but fewer columns and rows. |

### Colours

Colours are specified as six-digit hex codes (same as HTML/CSS), with no `#` prefix.

| Option                    | Example             | Description                                                                                                                                                                                                                    |
|---------------------------|---------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-bg`<br/>`--bgcolour <RRGGBB>` | `--bgcolour 0d1117` | Solid background colour. Used when no background image is set. Black (`000000`) by default.                                                                                                                                    |
| `-fg`<br/>`--fgcolour <RRGGBB>` | `--fgcolour c9d1d9` | Override the foreground (text) colour. When set, this colour is used for **all** text, ignoring ANSI colour codes from programs. Useful for a consistent look. When not set, programs control their own text colours normally. |

### Terminal behaviour

| Option                | Example             | Description                                                                                                                                                    |
|-----------------------|---------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-sb`<br/>`--scrollback <n>` | `--scrollback 2000` | Number of lines to keep in scroll history. Default is 512. Higher values use more memory. Set to `1` to disable scrollback.                                    |
| `-ro`<br/>`--readonly`     | `--readonly`        | Read-only mode. muTerm displays output but ignores all input. The OSK is hidden and the `[RO]` badge is shown in the corner. Useful for monitoring log output. |

### Running a specific program

To run something other than your default shell, add `--` followed by the command and its arguments:

```
muterm -- top
muterm -- python3 /opt/myscript.py
muterm -- bash -c "tail -f /var/log/syslog"
```

You can also just put the command at the end without `--`, as long as it doesn't start with `-`:

```
muterm top
```

---

## Font Suggestions

muTerm works with any monospace TrueType font. The font determines how many columns and rows fit on screen.
smaller fonts give more text, larger fonts are easier to read.

### General recommendations

- **Font size 14–16** - good for most screen sizes at 640x480
- **Font size 18–24** - better readability on smaller physical screens
- **Font size 28** - automatically applied on high DPI hardware by MustardOS

### The Oldschool PC Font Resource

[**The Oldschool PC Font Resource**](https://int10h.org/oldschool-pc-fonts/) (int10h.org) by VileR is a superb collection of faithfully reconstructed fonts from
classic DOS-era hardware. All fonts are free to use. The TTF versions work directly with muTerm.

These are particularly well suited to muTerm:

| Font name               | Style                          | Why it works well                                                                                             |
|-------------------------|--------------------------------|---------------------------------------------------------------------------------------------------------------|
| **PxPlus IBM VGA8**     | Classic IBM PC VGA             | The definitive DOS terminal look. Clean at small sizes, excellent box-drawing character support. Try size 16. |
| **PxPlus IBM VGA9**     | IBM VGA 9-wide variant         | Slightly wider than VGA8, easier to read. Good at size 14–16.                                                 |
| **PxPlus IBM EGA8**     | EGA era                        | Slightly more compact than VGA. Great for fitting more columns on smaller screens.                            |
| **PxPlus ToshibaTxL1**  | Toshiba laptop                 | Elegant, slightly condensed. Fits well at sizes 14–18.                                                        |
| **PxPlus HP 100LX 6x8** | HP palmtop                     | Very compact. Excellent if you want maximum columns on a small screen.                                        |
| **PxPlus Verite 9x14**  | Verite graphics card           | Tall, very legible. Good choice at size 14 on larger displays.                                                |
| **PxPlus IBM MDA**      | IBM Monochrome Display Adapter | The original monochrome terminal aesthetic. Strikes a great balance between density and readability.          |
| **PxPlus ATI 8x16**     | ATI graphics card              | Slightly rounder than IBM fonts. Pleasant at size 16.                                                         |
| **PxPlus Compaq 8x16**  | Compaq portable                | A popular alternative to IBM VGA. Slightly different proportions, very readable.                              |

Download the TTF pack from [https://int10h.org/oldschool-pc-fonts/download/](https://int10h.org/oldschool-pc-fonts/download/) and copy your chosen font to your
device, for example:

```
/opt/muos/share/font/PxPlus_IBM_VGA8.ttf
```

Then in your `muterm.conf`:

```ini
font_path = /opt/muos/share/font/PxPlus_IBM_VGA8.ttf
font_size = 16
```

### Other good monospace fonts

- **Terminus** - extremely clean bitmap-style font, excellent legibility at small sizes
- **Hack** - modern, designed for code, good Unicode coverage
- **JetBrains Mono** - popular developer font, works well at larger sizes
- **Cozette** - tiny pixel font, great for high information density
- **Spleen** - bitmap-style, designed for small displays

---

## Technical Reference

This section covers the internal workings, build process, and platform details for developers and advanced users.

---

### Building from source

#### Dependencies

| Library        | Package name (Debian/Ubuntu) | Purpose                          |
|----------------|------------------------------|----------------------------------|
| SDL2           | `libsdl2-dev`                | Window, renderer, input events   |
| SDL2_ttf       | `libsdl2-ttf-dev`            | TrueType font rendering          |
| SDL2_image     | `libsdl2-image-dev`          | PNG background image loading     |
| libc / libutil | *(system)*                   | `openpty`, `fork`, POSIX signals |

#### Compile

```bash
gcc -O2 -o muterm \
    config.c vt.c render.c osk.c main.c \
    $(sdl2-config --cflags --libs) \
    -lSDL2_ttf -lSDL2_image -lutil \
    -Wall -Wextra
```

Or with a Makefile:

```makefile
CC      = gcc
CFLAGS  = -O2 -Wall -Wextra $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image -lutil

muterm: config.c vt.c render.c osk.c main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
```

#### Cross-compiling for MustardOS (ARM)

```bash
arm-linux-gnueabihf-gcc -O2 -o muterm \
    config.c vt.c render.c osk.c main.c \
    -I/path/to/sysroot/usr/include/SDL2 \
    -L/path/to/sysroot/usr/lib \
    -lSDL2 -lSDL2_ttf -lSDL2_image -lutil \
    -lm -lpthread
```

---

### Source file overview

| File            | Responsibility                                                                                              |
|-----------------|-------------------------------------------------------------------------------------------------------------|
| `config.c / .h` | Config file parsing, MustardOS device/global config reading, CLI defaults                                   |
| `vt.c / .h`     | Full VT/xterm-compatible terminal emulator - UTF-8, SGR colours, scrollback, alternate screen, ACS graphics |
| `render.c / .h` | SDL2 rendering - glyph cache, soft-rendered box/block characters, cursor blink, overlay badges              |
| `osk.c / .h`    | On-screen keyboard - three layers (lower/upper/Ctrl), gamepad navigation, key repeat, axis/hat input        |
| `main.c`        | Entry point - SDL init, PTY spawn, event loop, display pipeline                                             |

---

### Terminal emulation

muTerm emulates an **xterm-256color** terminal. The `TERM` environment variable is
set to `xterm-256color` and `COLORTERM` to `truecolor` in the child process.

**Supported escape sequences:**

- **Cursor movement**: `CUU` `CUD` `CUF` `CUB` `CUP` `HVP` `CHA` `VPA` `CNL` `CPL`
- **Erase**: `ED` (J), `EL` (K), `ECH` (X)
- **Scroll**: `SU` (S), `SD` (T), `IL` (L), `DL` (M), scroll regions via `DECSTBM` (r)
- **Insert/delete**: `ICH` (@), `DCH` (P)
- **SGR**: bold, underline, reverse, 8-colour, 256-colour (`38;5;n`), true colour (`38;2;r;g;b`)
- **Modes**: `DECCKM` (cursor keys app mode), `DECTCEM` (cursor visibility), LNM (linefeed mode)
- **Alternate screen**: `?47h/l`, `?1047h/l`, `?1049h/l`
- **Character sets**: G0/G1 designation (`(` `)` sequences), SI/SO shift, DEC Special Graphics (ACS line-drawing)
- **Misc**: `RIS` (reset), `DECSC`/`DECRC` (save/restore cursor), reverse index (`RI`)

**Soft-rendered characters** (drawn directly, not via font):

- Box drawing: U+2500–U+257F (including heavy and double-line variants)
- Block elements: U+2580–U+259F (halves, quarters, shade patterns)
- Geometric shapes: U+25A0, U+25CF
- Braille/scanlines: U+23BA–U+23BD
- Control pictures: U+2409–U+2424 (HT, LF, VT, FF, CR, NL glyphs)
- Middle dot: U+00B7

---

### Glyph cache

Rendered glyphs are cached in a hash table (`GLYPH_BUCKETS = 1024`, `GLYPH_MAX_ENTRIES = 8192`). The cache key is `(codepoint, fg colour, style)`. When the
cache is full it is cleared entirely and repopulated. The cache is freed on exit via `render_glyph_cache_clear()`.

---

### PTY and child process

muTerm uses `openpty()` + `fork()` to create a pseudo-terminal. The child process:

- Calls `setsid()` and `TIOCSCTTY` to become session leader with a controlling terminal
- Sets `TERM=xterm-256color`, `COLORTERM=truecolor`, `COLUMNS`, `LINES`, and `HOME` (if unset)
- Executes either the command passed after `--`, or the user's `$SHELL`, or `/bin/sh`

The master PTY fd is set non-blocking. The main loop polls it with zero timeout and reads in a tight loop
until `EAGAIN`. `SIGCHLD` is caught via `sigaction` to detect child exit. `SIGPIPE` is ignored to prevent
crashes if the PTY write occurs after the child exits.

---

### On-screen keyboard

The OSK has three distinct **layers**:

| Layer  | Contents                                      |
|--------|-----------------------------------------------|
| `abc`  | Lowercase letters, digits, common punctuation |
| `ABC`  | Uppercase letters, shifted punctuation        |
| `Ctrl` | F1–F12, Ctrl+A through Ctrl+_ sequences       |

All three layers share a common bottom navigation row: `Tab`, `Esc`, `Ctrl`, `Alt`, arrow keys.

**Ctrl** and **Alt** are sticky modifier keys - press once to arm, press a character key to send the combined sequence.
Ctrl converts a single-character sequence to its control-code equivalent (`a`→`\x01`, etc.). Alt prepends `\x1B`.

Arrow key sequences respect the terminal's **cursor keys application mode** (`DECCKM`): `\x1B[A`–`D` in normal mode, `\x1BOA`–`D` in application mode.

Key **repeat** behaviour: after `KEY_REPEAT_DELAY` ms (350 ms) the action fires again every `KEY_REPEAT_RATE` ms (70 ms).
This applies to A (key press), B (backspace), and Y (space).

**OSK states** cycle through:

| State           | Description                                                                 |
|-----------------|-----------------------------------------------------------------------------|
| `BOTTOM_OPAQUE` | OSK fills the lower portion; terminal rows above it are reduced to fit      |
| `BOTTOM_TRANS`  | OSK is semi-transparent at the bottom; full terminal rows visible behind it |
| `TOP_OPAQUE`    | OSK fills the upper portion                                                 |
| `TOP_TRANS`     | OSK is semi-transparent at the top                                          |
| `HIDDEN`        | OSK hidden; full terminal visible                                           |

---

### Display pipeline

Each frame (~33 ms / 30 fps cap):

1. SDL events are processed
2. PTY data is read and fed to the VT emulator
3. If the screen is dirty, the OSK is visible, or the cursor needs to blink: render to an off-screen `SDL_Texture` target
4. The texture is composited to the window using `SDL_RenderCopyExF` with zoom (`SDL_FRect`) and rotation (angle)
5. The OSK is rendered on top if visible
6. `SDL_RenderPresent` flips to screen

The render target texture dimensions are exactly `TERM_COLS x CELL_WIDTH` by `TERM_ROWS x CELL_HEIGHT`. Zoom and underscan are applied only at the final blit
stage, keeping the terminal buffer at a fixed logical resolution.

---

### MustardOS specific device config paths

| Path                                    | Contents                                            |
|-----------------------------------------|-----------------------------------------------------|
| `/opt/muos/device/config/screen/width`  | Physical screen width in px                         |
| `/opt/muos/device/config/screen/height` | Physical screen height in px                        |
| `/opt/muos/device/config/screen/zoom`   | Display zoom factor                                 |
| `/opt/muos/device/config/screen/rotate` | Screen rotation (0–3)                               |
| `/opt/muos/device/config/board/name`    | Board name (e.g. `tui-brick` → forces font size 28) |
| `/opt/muos/config/settings/hdmi/scan`   | HDMI underscan flag (`1` = on)                      |

---

### Joystick / controller mapping

muTerm supports both SDL GameController API (mapped controllers) and raw joystick events (for devices not in SDLs database).
GameController events take priority when a mapped controller is open.

**Raw joystick button mapping (MustardOS defaults):**

| Button index | Action                 |
|--------------|------------------------|
| 1            | Page down              |
| 2            | Page up                |
| 3            | Select / press key (A) |
| 4            | Backspace (B)          |
| 5            | Space (Y)              |
| 7            | Previous layer (L1)    |
| 8            | Next layer (R1)        |
| 9            | Toggle OSK (Select)    |
| 10           | Enter (Start)          |
| 11           | Quit (Menu/Guide)      |

* Left stick axis 0 = horizontal navigation, axis 1 = vertical navigation.
* Dead zone is roughly `16000`.
* Hat events are also handled for D-pads not reported as axes.

A USB keyboard can also be used via a USB-C dongle if using the OSK is too much hard work.
