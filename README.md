# muTerm

A fast, portable virtual terminal built for **MustardOS** handheld devices. Run a full shell, execute commands, and navigate your device
entirely from a gamepad-driven on-screen keyboard, or plug in a USB keyboard if you prefer.

---

## What is muTerm?

muTerm is a terminal emulator that runs directly on your MustardOS device. It opens a real shell (bash, sh, or any program you choose), displays its output on
screen, and lets you type using a gamepad-driven on-screen keyboard (OSK). It supports full colour, scrollback history, background images, display rotation,
zoom, and sixel graphics. Everything you need for a proper terminal experience on a handheld.

It also works as a **read-only viewer**: pipe a program's output into it and watch it scroll in real time, with no input needed.

---

## Quick Start

### What you need

- The `muterm` binary in your path, or called directly
- A monospace TTF font (see [Font Suggestions](#font-suggestions) below)

### Running muTerm

```
muterm
muterm -- htop
muterm -- bash -c "ls -la /opt"
```

### Basic gamepad controls

| Button           | Action                                                                           |
|------------------|----------------------------------------------------------------------------------|
| **Select**       | Cycle the on-screen keyboard (bottom > transparent > top > transparent > hidden) |
| **Menu / Guide** | Quit muTerm                                                                      |

**When the OSK is visible:**

| Button                   | Action                    |
|--------------------------|---------------------------|
| **D-Pad / Left stick**   | Move the key cursor       |
| **A / Left stick click** | Press the highlighted key |
| **B**                    | Backspace                 |
| **Y**                    | Space                     |
| **Start**                | Enter                     |
| **L1 / R1**              | Switch keyboard layer     |
| **L2 / R2**              | Scroll terminal history   |

**When the OSK is hidden:**

| Button        | Action                  |
|---------------|-------------------------|
| **D-Pad U/D** | Previous / next command |
| **D-Pad L/R** | Cursor movement         |
| **L2 / R2**   | Scroll terminal history |

---

## Configuration

muTerm reads its settings from config files, applied in this order (later files override earlier ones):

1. `/opt/muos/device/config` — device-specific screen size and zoom *(MustardOS only)*
2. `/opt/muos/config` — global MustardOS settings *(MustardOS only)*
3. `/opt/muos/share/conf/muterm.conf` — system-wide muTerm defaults
4. `~/.config/muterm/muterm.conf` — your personal overrides

A minimal `muterm.conf` looks like this:

```ini
font_path = /opt/muos/share/font/muterm.ttf
font_size = 18
scrollback = 1024
bg_colour = 1a1a2e
fg_colour = e0e0ff
```

All options available on the command line can also be placed in a config file using `key = value` format, one per line. Lines starting with `#` are comments. An
annotated example config file is provided as `muterm_conf.example`.

---

## Command Line Options

Command-line values always win over config file values.

### Display

| Option                  | Example           | Description                                                                         |
|-------------------------|-------------------|-------------------------------------------------------------------------------------|
| `-w` / `--width <px>`   | `--width 640`     | Window width in pixels. Defaults to the MustardOS device config value, or 640.      |
| `-h` / `--height <px>`  | `--height 480`    | Window height in pixels. Defaults to the MustardOS device config value, or 480.     |
| `--zoom <factor>`       | `--zoom 1.5`      | Scale the entire terminal. `1.0` is normal size. `1.5` makes everything 50% larger. |
| `--rotate <0–3>`        | `--rotate 1`      | Rotate the display. `0` = normal, `1` = 90°, `2` = 180°, `3` = 270°.                |
| `--underscan`           |                   | Add a 16-pixel border inset on all sides for HDMI displays that crop the edges.     |
| `--no-underscan`        |                   | Force underscan off even if the MustardOS global config enables it.                 |
| `-i` / `--image <file>` | `--image /bg.png` | PNG background image, stretched to fill the window.                                 |

### Font

| Option                      | Example                          | Description                                                              |
|-----------------------------|----------------------------------|--------------------------------------------------------------------------|
| `-f` / `--font <file>`      | `--font /opt/share/font/vga.ttf` | Path to a monospace TrueType font.                                       |
| `--font-bold <file>`        |                                  | Explicit bold variant. If omitted, SDL_ttf bases bold from the base.     |
| `--font-italic <file>`      |                                  | Explicit italic variant. If omitted, SDL_ttf bases italic from the base. |
| `--font-bold-italic <file>` |                                  | Explicit bold+italic variant.                                            |
| `-s` / `--size <pt>`        | `--size 16`                      | Font size in points.                                                     |
| `--font-hinting <mode>`     | `--font-hinting mono`            | Hinting mode: `normal`, `light`, `mono`, or `none`.                      |

Config file equivalents: `font_path`, `font_path_bold`, `font_path_italic`, `font_path_bold_italic`, `font_size`, `font_hinting`.

### Colours

Colours are six-digit hex values (RRGGBB), with or without a leading `#`.

| Option                        | Example             | Description                                                                                                                                                |
|-------------------------------|---------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-bg` / `--bgcolour <RRGGBB>` | `--bgcolour 0d1117` | Solid background colour. Black by default.                                                                                                                 |
| `-fg` / `--fgcolour <RRGGBB>` | `--fgcolour c9d1d9` | Override the foreground colour. When set, **all** text is drawn in this colour, ignoring ANSI colour codes. Leave unset to let programs use their colours. |

### Terminal Behaviour

| Option                     | Example                       | Description                                                                                                                                                 |
|----------------------------|-------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-sb` / `--scrollback <n>` | `--scrollback 2000`           | Number of lines to keep in scroll history. Default is 512.                                                                                                  |
| `--sb-path <file>`         | `--sb-path /tmp/muterm.cache` | Path for scrollback persistence. Default: `/tmp/muterm.cache`.                                                                                              |
| `--no-sb-persist`          |                               | Disable scrollback save/load for this session.                                                                                                              |
| `-ro` / `--readonly`       |                               | Read-only mode. muTerm displays output but ignores all input. The OSK is hidden and a `[RO]` badge is shown. Useful for monitoring logs.                    |
| `--force-redraw`           |                               | Force a full redraw every frame, bypassing dirty-row optimisation. Useful for programs like `htop`, `btop`, `vim`, or `tmux` that refresh the whole screen. |

### On-Screen Keyboard

| Option                | Example                              | Description                                                                                |
|-----------------------|--------------------------------------|--------------------------------------------------------------------------------------------|
| `--osk-layout <file>` | `--osk-layout /opt/muterm/my.layout` | Load extra OSK layers from a layout file. See [OSK Layout Files](#osk-layout-files) below. |

Config file key: `osk_layout_path`.

### Input Repeat

All values are in milliseconds.

| Option              | Config key          | Default | Description                                                |
|---------------------|---------------------|---------|------------------------------------------------------------|
| `--key-delay <ms>`  | `key_repeat_delay`  | `350`   | Hold time before an OSK button (A, B, Y) starts repeating. |
| `--key-rate <ms>`   | `key_repeat_rate`   | `70`    | Interval between repeated OSK presses while held.          |
| `--dpad-delay <ms>` | `dpad_repeat_delay` | `300`   | Hold time before D-Pad navigation starts repeating.        |
| `--dpad-rate <ms>`  | `dpad_repeat_rate`  | `80`    | Interval between repeated D-Pad steps while held.          |

### Special Options

| Option          | Description                                                                                                   |
|-----------------|---------------------------------------------------------------------------------------------------------------|
| `--ignore-muos` | Skip all MustardOS device and global config files. Still reads `muterm.conf`. **Must be the first argument.** |
| `--version`     | Print the muTerm version and exit.                                                                            |
| `--help` / `-?` | Print usage summary and exit.                                                                                 |

### Running a Specific Program

Add `--` followed by the command and its arguments:

```
muterm -- top
muterm -- python3 /opt/myscript.py
muterm -- bash -c "tail -f /var/log/syslog"
```

You can also put the command at the end without `--`, as long as it does not start with `-`:

```
muterm top
```

---

## OSK Layout Files

You can define additional on-screen keyboard layers in a plain-text layout file and load them with `--osk-layout` or `osk_layout_path`. Loaded layers are
appended after the built-in `abc`, `ABC`, and `Ctrl` layers and are reachable by cycling with **L1/R1**.

### Format

```
# Comments start with #

[LayerName]
label | send | width

label | send | width
label | send | width
```

- `[LayerName]` — starts a new layer with the given name (shown in the OSK header).
- Each line defines one key: `label | send | width`.
    - **label** — text shown on the key (UTF-8 supported).
    - **send** — byte sequence sent when pressed. Supports `\t` `\r` `\n` `\\` `\xHH`.
    - **width** — optional column span (default `1`).
- Blank lines separate rows. Up to 4 data rows per layer; a navigation row (Tab, Esc, arrows, etc.) is always appended automatically.
- Up to 12 keys per row. Extra keys are silently ignored.
- Up to 13 additional layers can be loaded (16 total including the 3 built-ins).

### Example

```text
# /opt/muterm/symbols.layout

[Symbol]
! | ! | 1
@ | @ | 1
# | # | 1
$ | $ | 1
% | % | 1
^ | ^ | 1

& | & | 1
* | * | 1
( | ( | 1
) | ) | 1
_ | _ | 1
+ | + | 1

{ | { | 1
} | } | 1
[ | [ | 1
] | ] | 1
; | ; | 1
: | : | 1

~ | ~ | 1
` | ` | 1
/ | / | 1
\ | \\ | 1
| | | | 1
" | " | 1
```

Load via config:

```ini
osk_layout_path = /opt/muterm/symbols.layout
```

Or on the command line:

```
muterm --osk-layout /opt/muterm/symbols.layout
```

---

## Font Suggestions

muTerm works with any monospace TrueType font. Smaller fonts give more columns and rows; larger fonts are easier to read.

### General Recommendations

- **Font size 14–16** — good for most screens at 640×480
- **Font size 18–24** — better readability on smaller physical screens
- **Font size 28** — automatically applied on high-DPI hardware by MustardOS

### Nerd Fonts

[**Nerd Fonts**](https://www.nerdfonts.com/) patch popular monospace fonts with thousands of extra icons including Powerline symbols, box-drawing extensions,
and developer icons, giving excellent coverage of the geometric and symbolic characters that muTerm renders via the font (triangles, arrows, shapes beyond the
soft-rendered range).

Recommended: **JetBrains Mono Nerd Font**, **Hack Nerd Font**, **DejaVu Sans Mono Nerd Font**.

### The Oldschool PC Font Resource

[**The Oldschool PC Font Resource**](https://int10h.org/oldschool-pc-fonts/) by VileR provides faithfully reconstructed fonts from classic DOS-era hardware. TTF
versions work directly with muTerm and include excellent box-drawing coverage.

| Font name               | Style                  | Why it works well                                                                |
|-------------------------|------------------------|----------------------------------------------------------------------------------|
| **PxPlus IBM VGA8**     | Classic IBM PC VGA     | Definitive DOS terminal look. Superb box-drawing support. Try size 16.           |
| **PxPlus IBM VGA9**     | IBM VGA 9-wide         | Slightly wider, easier to read at size 14–16.                                    |
| **PxPlus IBM EGA8**     | EGA era                | More compact than VGA. Great for fitting more columns on small screens.          |
| **PxPlus ToshibaTxL1**  | Toshiba laptop         | Elegant, slightly condensed. Fits well at sizes 14–18.                           |
| **PxPlus HP 100LX 6x8** | HP palmtop             | Very compact. Excellent for maximum columns on a small screen.                   |
| **PxPlus IBM MDA**      | IBM Monochrome Display | Original monochrome terminal aesthetic. Great balance of density and legibility. |

Download from [int10h.org](https://int10h.org/oldschool-pc-fonts/download/), copy to your device, then in `muterm.conf`:

```ini
font_path = /opt/muos/share/font/PxPlus_IBM_VGA8.ttf
font_size = 16
```

### Other Good Monospace Fonts

- **Terminus** — extremely clean bitmap-style, excellent legibility at small sizes
- **Hack** — modern, designed for code, good Unicode coverage
- **Cozette** — tiny pixel font, great for high information density
- **Spleen** — bitmap-style, designed for small displays

---

## Technical Reference

This section covers internal workings, the build process, and platform details for developers and advanced users.

---

### Building from Source

#### Dependencies

| Library        | Package name (Debian/Ubuntu) | Purpose                          |
|----------------|------------------------------|----------------------------------|
| SDL2           | `libsdl2-dev`                | Window, renderer, input events   |
| SDL2_ttf       | `libsdl2-ttf-dev`            | TrueType font rendering          |
| SDL2_image     | `libsdl2-image-dev`          | PNG background image loading     |
| libc / libutil | *(system)*                   | `openpty`, `fork`, POSIX signals |

#### Compile

The Makefile builds all `.c` files in the source directory automatically:

```bash
make
```

To cross-compile for MustardOS (AArch64):

```bash
make CC=aarch64-linux-gcc TARGET=../frontend/bin/muterm
```

This produces a stripped binary at the target path. The Makefile uses `-O2 -pipe -Wall -Wextra -std=c11` and links `-lSDL2 -lSDL2_ttf -lSDL2_image`.

To also build the `png2sixel` converter:

```bash
gcc -O2 -o png2sixel png2sixel.c -lpng -lm
```

To build muTerm manually without the Makefile:

```bash
gcc -O2 -std=c11 -o muterm \
    config.c vt.c render.c osk.c input.c sixel.c main.c \
    $(sdl2-config --cflags --libs) \
    -lSDL2_ttf -lSDL2_image -lutil \
    -Wall -Wextra
```

---

### Source File Overview

| File            | Responsibility                                                                                                |
|-----------------|---------------------------------------------------------------------------------------------------------------|
| `main.c`        | Entry point — SDL init, font loading, PTY spawn, event loop, display pipeline, fade in/out                    |
| `vt.c / .h`     | VT/xterm-compatible terminal emulator — UTF-8, SGR, scrollback, alternate screen, ACS, sixel DCS dispatch     |
| `render.c / .h` | SDL2 rendering — glyph cache, soft-rendered box/block/braille chars, sixel blit, cursor blink, overlay badges |
| `sixel.c / .h`  | Sixel graphics decoder — multi-image list (up to 16), palette, pixel buffer, scroll tracking                  |
| `osk.c / .h`    | On-screen keyboard — built-in and dynamic layers, gamepad navigation, key repeat, axis/hat input              |
| `input.c / .h`  | SDL event handling, D-Pad repeat, GameController/joystick mapping                                             |
| `config.c / .h` | Config file parsing, MustardOS device/global config reading, CLI defaults                                     |

---

### Terminal Emulation

muTerm emulates an **xterm-256color** terminal. The child process receives `TERM=xterm-256color` and `COLORTERM=truecolor`.

* **Cursor movement:** `CUU` `CUD` `CUF` `CUB` `CUP` `HVP` `CHA` `VPA` `CNL` `CPL`
* **Erase:** `ED` (J — modes 0/1/2), `EL` (K — modes 0/1/2), `ECH` (X)
* **Scroll:** `SU` (S), `SD` (T), `IL` (L), `DL` (M), scroll regions via `DECSTBM` (r)
* **Insert / delete:** `ICH` (@), `DCH` (P)

**SGR attributes:**

| Code       | Effect            | Off code | Description                      |
|------------|-------------------|----------|----------------------------------|
| 0          | Reset all         | —        |                                  |
| 1          | Bold              | 22       | Also clears dim                  |
| 2          | Dim               | 22       | 50% alpha on glyph texture       |
| 3          | Italic            | 23       |                                  |
| 4          | Underline         | 24       |                                  |
| 7          | Reverse video     | 27       | Swaps fg/bg                      |
| 9          | Strikethrough     | 29       | Horizontal line at cell midpoint |
| 30–37      | Normal foreground | 39       | 8 ANSI colours                   |
| 40–47      | Normal background | 49       | 8 ANSI colours                   |
| 90–97      | Bright foreground |          |                                  |
| 100–107    | Bright background |          |                                  |
| 38;5;n     | 256 colour fg     |          |                                  |
| 48;5;n     | 256 colour bg     |          |                                  |
| 38;2;r;g;b | True colour fg    |          |                                  |
| 48;2;r;g;b | True colour bg    |          |                                  |

* **Modes:** `DECCKM` (?1), `DECAWM` (?7 — auto-wrap on/off), `DECTCEM` (?25 — cursor visibility), LNM (mode 20)
* **Alternate screen:** `?47h/l`, `?1047h/l`, `?1049h/l`
* **Cursor save/restore:** `DECSC`/`DECRC` (`ESC 7`/`ESC 8`), `CSI s`/`CSI u`
* **Character sets:** G0/G1 designation (`ESC ( char`, `ESC ) char`), SI/SO shift
* **OSC:** `ESC ] 0 ; title BEL` / `ESC ] 2 ; title BEL` — sets the SDL window title
* **DCS / Sixel:** `ESC P … q … ESC \` — sixel graphics (see [Sixel Graphics](#sixel-graphics) below)
* **Misc:** `RIS` (`ESC c`), reverse index (`ESC M`), index (`ESC D`), next line (`ESC E`)

**Known limitations:**

- Mouse reporting (`?1000h`, `?1006h`) — not implemented
- Bracketed paste (`?2004h`) — mode accepted but markers not sent
- `DECSCUSR` cursor shape — cursor is always a blinking block
- `REP` (`ESC [ n b`) — silently ignored
- Terminal resize / `SIGWINCH` — size is fixed at launch; runtime resize not propagated

---

### Soft-Rendered Characters

The following Unicode ranges are drawn directly by muTerm's pixel renderer, guaranteeing correct rendering regardless of font coverage:

| Range          | Characters                                                  |
|----------------|-------------------------------------------------------------|
| U+2500–U+257F  | Box drawing — single, double, heavy lines, all connectors   |
| U+2580–U+259F  | Block elements — all 32: halves, eighths, quadrants, shades |
| U+25A0, U+25CF | Black square (■) and black circle (●)                       |
| U+23BA–U+23BD  | Horizontal scan lines                                       |
| U+2409–U+2424  | Control pictures (HT, LF, VT, FF, CR, NL glyphs)            |
| U+2800–U+28FF  | Braille patterns — all 256                                  |
| U+00B7         | Middle dot (·)                                              |

All other characters are rendered via the configured font. Characters missing from the font show as a blank cell — install a Nerd Font or a font with broader
Unicode coverage for triangles, arrows, mathematical symbols, and CJK characters.

---

### Font Slots

muTerm maintains **8 internal font slots**, one for each combination of bold (bit 0), underline (bit 1), and italic (bit 2). Dim and strikethrough are rendering
effects applied at blit time and do not consume separate font slots.

If explicit font variant files are provided (`font_path_bold`, `font_path_italic`, `font_path_bold_italic`), those files are opened directly. Otherwise SDL_ttf
bases the style from the base font automatically, which works well for most purposes.

Font hinting is applied via `TTF_SetFontHinting` per slot:

| Mode     | Effect                                                        |
|----------|---------------------------------------------------------------|
| `normal` | Default FreeType hinting — good for most smooth fonts         |
| `light`  | Lighter hinting, preserves stroke shape                       |
| `mono`   | Monochrome hinting, crisp pixel edges — best for bitmap fonts |
| `none`   | No hinting — useful at very high DPI or pixel-perfect fonts   |

---

### Sixel Graphics

muTerm supports DCS sixel streams (`ESC P … q … ESC \`). Up to **16 images** can be visible simultaneously on screen. Each image is decoded into an RGBA pixel
buffer and blitted onto the render target every frame on top of the cell grid. Images track with scrolling text and are freed automatically when they scroll off
the top of the viewport.

---

### Glyph Cache

Rendered font glyphs are cached in a hash table (`GLYPH_BUCKETS = 1024`, `GLYPH_MAX_ENTRIES = 8192`). The cache key is `(codepoint, fg colour, style bitmask)`.
When the cache is full, the oldest entry is evicted in O(1) using a FIFO ring buffer. Soft-rendered characters (box, block, braille) are also stored in the same
cache as SDL textures. The cache is cleared on exit via `render_glyph_cache_clear()`.

---

### Dirty-Row Tracking

muTerm tracks which screen rows have changed since the last frame using a per-row `Uint8` dirty flag array. `render_screen` skips rows whose flag is clear, so
frames where only a few lines changed avoid redrawing the entire terminal. For cursor blink frames (no other dirty content), only the cursor row is marked
dirty, keeping the per-frame repaint cost to a single row strip fill plus glyph redraw.

---

### Display Pipeline

Each frame (~33 ms, 30 fps cap):

1. SDL events are processed (keyboard, controller, joystick)
2. All available PTY bytes are read into a 64 KB batch buffer and fed to the VT emulator in a single `vt_feed()` call
3. Child process state is checked via `SIGCHLD` / `waitpid`
4. If the screen is dirty → `render_screen` repaints all dirty rows, draws the cursor highlight, then blits all sixel images on top
5. If only the cursor blink needs updating → mark the cursor row dirty and run `render_screen` for just that row (no separate `render_cursor_blink` call)
6. If the OSK is visible → render the OSK overlay on top of the terminal texture
7. The render target texture is composited to the window using `SDL_RenderCopyExF` with zoom (`SDL_FRect`) and rotation
8. Fade-in (first content frame) and fade-out (on exit) are applied as alpha overlays via `SDL_RenderFillRect`
9. `SDL_RenderPresent` flips to screen

The render target dimensions are exactly `TERM_COLS × CELL_WIDTH` by `TERM_ROWS × CELL_HEIGHT` pixels. Zoom, rotation, and underscan are applied only at the
final blit stage, keeping the terminal buffer at a fixed logical resolution.

---

### PTY and Child Process

muTerm uses `openpty()` + `fork()` to create a pseudo-terminal. The child process:

- Calls `setsid()` and `TIOCSCTTY` to become session leader with a controlling terminal
- Sets `TERM=xterm-256color`, `COLORTERM=truecolor`, `COLUMNS`, `LINES`, and `HOME` (if unset)
- Executes either the command passed after `--`, or `cfg.shell`, or `$SHELL`, or `/bin/sh`

The master PTY fd is set non-blocking. `SIGCHLD` is caught via `sigaction(SA_NOCLDSTOP)` to detect child exit. `SIGPIPE` is ignored.

---

### Scrollback Persistence

muTerm saves the scrollback buffer on exit and restores it on the next launch using a compact binary format (`SB_MAGIC` header, cols, count, 14 bytes per cell).
The default cache path is `/tmp/muterm.cache`. If the terminal column count changes between sessions, the old cache is silently discarded.

```ini
scrollback_path = /home/user/.muterm-history
```

```
muterm --no-sb-persist    # disable for this session
```

---

### OSK Details

The OSK has three built-in layers plus any extra layers loaded from a layout file:

| Layer  | Contents                                      |
|--------|-----------------------------------------------|
| `abc`  | Lowercase letters, digits, common punctuation |
| `ABC`  | Uppercase letters, shifted punctuation        |
| `Ctrl` | F1–F12, Ctrl+A through Ctrl+_ sequences       |

All layers share a common bottom navigation row: Tab, Esc, Ctrl, Alt, arrow keys.

**Ctrl** and **Alt** are sticky modifier keys — press once to arm, press a character key to send the combined sequence. Ctrl converts to its control-code
equivalent (`a` → `\x01`, etc.). Alt prepends `\x1B`.

Arrow key sequences respect `DECCKM`: `\x1B[A`–`D` in normal mode, `\x1BOA`–`D` in application mode.

**OSK states** cycle through:

| State           | Description                                                            |
|-----------------|------------------------------------------------------------------------|
| `BOTTOM_OPAQUE` | OSK fills the lower portion; terminal rows above are reduced to fit    |
| `BOTTOM_TRANS`  | OSK is semi-transparent at the bottom; full terminal visible behind it |
| `TOP_OPAQUE`    | OSK fills the upper portion                                            |
| `TOP_TRANS`     | OSK is semi-transparent at the top                                     |
| `HIDDEN`        | OSK hidden; full terminal visible                                      |

---

### MustardOS Device Config Paths

| Path                                    | Contents                                     |
|-----------------------------------------|----------------------------------------------|
| `/opt/muos/device/config/screen/width`  | Physical screen width in px                  |
| `/opt/muos/device/config/screen/height` | Physical screen height in px                 |
| `/opt/muos/device/config/screen/zoom`   | Display zoom factor                          |
| `/opt/muos/device/config/screen/rotate` | Screen rotation (0–3)                        |
| `/opt/muos/device/config/board/name`    | Board name (`tui-brick` forces font size 28) |
| `/opt/muos/config/settings/hdmi/scan`   | HDMI underscan flag (`1` = on)               |

---

### Controller Mapping

muTerm supports both the SDL GameController API (for mapped controllers) and raw joystick events (for devices not in SDL's database). GameController events take
priority when a mapped controller is open.

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
| 11           | Quit (Menu / Guide)    |

Left stick axis 0 = horizontal OSK navigation, axis 1 = vertical.
Dead zone is ~16 000. Hat events are handled for D-pads not reported as axes.
A USB keyboard works via a USB-C dongle.

---

## Licence

muTerm is released under the [GNU General Public License v3.0](LICENSE).
