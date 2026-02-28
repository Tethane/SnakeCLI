# 🐍 Snake

A terminal Snake game written in C — rendered with ANSI/Unicode,
playable at 60 FPS, with persistent high-score caching.

```
 ██████╗ ███╗  ██╗ █████╗ ██╗  ██╗███████╗
██╔════╝ ████╗ ██║██╔══██╗██║ ██╔╝██╔════╝
 █████╗  ██╔██╗██║███████║█████╔╝ █████╗
     ██╗ ██║╚████║██╔══██║██╔═██╗ ██╔══╝
██████╔╝ ██║ ╚███║██║  ██║██║  ██╗███████╗
╚═════╝  ╚═╝  ╚══╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝
```

## Requirements

- Linux (Ubuntu 20.04+)
- GCC or Clang
- CMake ≥ 3.10
- A terminal emulator that supports UTF-8 and ANSI colour codes
  (virtually all modern terminals do — gnome-terminal, kitty, alacritty, etc.)

## Quick Start

```bash
chmod +x ~/.fun/snake/snake
~/.fun/snake/snake
```

The launcher auto-compiles the project with CMake on the first run.
Subsequent launches skip the build step unless source files have changed.

## Controls

| Action       | Keys              |
| ------------ | ----------------- |
| Move up      | `W` / `↑`         |
| Move down    | `S` / `↓`         |
| Move left    | `A` / `←`         |
| Move right   | `D` / `→`         |
| Pause / Quit | `Q` / `ESC`       |
| Select menu  | `Enter` / `Space` |

## Gameplay

- Snake starts with a length of **4**.
- Eat the red `◆◆` apple to grow by 1 and increase your score.
- Hitting a wall or yourself ends the game.
- Your **high score** is saved between sessions in `cache/highscore`.

## Project Layout

```
~/.fun/snake/
├── snake              # Bash launcher (build + run)
├── CMakeLists.txt     # CMake build definition
├── README.md
├── cache/
│   └── highscore      # Persisted best score (plain integer)
├── build/             # CMake build artefacts (auto-created)
├── bin/
│   └── snake          # Compiled binary (auto-created)
└── src/
    ├── main.c         # Entry point, screen state machine, game loop
    ├── game.c / .h    # Snake ring-buffer, apple logic, score I/O
    ├── render.c / .h  # ANSI frame renderer, all screens
    └── input.c / .h   # termios raw mode, non-blocking key reads
```

## Technical Notes

- **Loop rate**: 60 FPS (`FRAME_US = 16 666 µs`).
- **Snake speed**: advances every 6 frames → 10 cells/second.
- **Rendering**: entire frame is built into a static 64 KB buffer and
  flushed with a single `fwrite` call per frame. The cursor jumps to
  `\033[H` (top-left) each frame — no full clear → zero flicker.
- **Snake storage**: ring buffer of `Point`s; head insertion and tail
  eviction are both O(1).
- **Input**: `termios` raw mode with `VMIN=0 / VTIME=0` for
  non-blocking reads; arrow-key escape sequences are parsed inline.
