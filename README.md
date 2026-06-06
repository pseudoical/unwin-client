# unwin

Unwin (stylized as unwin) is a minimal [Kirka.io](https://kirka.io/) client for Linux written in C using Xlib and the [Chromium Embedded Framework (CEF)](#chromium-embedded-framework-cef).

## Features

- [JavaScript, CSS, and HTML injection](#javascript-css-and-html-injection)
- [Resource swapping](#resource-swapping)
- [Command-line flags](#passing-chromium-flags)

This project is intended to remain small and is unlikely to receive significant features/updates.

## Requirements

- Linux (x86_64)
- X11
- glibc

All [Chromium Embedded Framework (CEF)](#chromium-embedded-framework-cef) libraries and resources are bundled with the application.

## Installation

1. Download the lastest version from the [Releases](https://github.com/pseudoical/unwin-client/releases/) page.
2. Extract the archive.
3. Try launching `unwin` by double-clicking it in your file manager.

If that does not work:

4. Open a terminal in the extracted directory.

5. Make the program executable:

```bash
chmod +x ./unwin
```

6. Run the program:

```bash
./unwin
```

## Low FPS workaround

Some systems may experience low frame rates.

Running:

```bash
xrandr
```

before launching the client may resolve the issue. The underlying cause is currently unknown.

### Additional information (not required)

Running:

```bash
xrandr
```

also displays an asterisk (`*`) next to the currently active refresh rate.

If the active refresh rate is not the one you expect, you can change it with:

```bash
xrandr --output DisplayName --mode Dimension --rate FPS
```

Example:

```bash
xrandr --output DisplayPort-0 --mode 1920x1080 --rate 120
```

## Common fixes

### Run the fix script

1. Make the script executable:

```bash
chmod +x ./fix.sh
```

2. Run the script:

```bash
./fix.sh
```

or run the commands inside the script manually.

### Check if you are using X11

Run:

```bash
echo "$XDG_SESSION_TYPE"
```

Expected output:

```text
x11
```

If the output is:

```text
wayland
```

then you are running a Wayland session. In that case, consider using Xwayland:

- https://wayland.freedesktop.org/docs/book/Xwayland.html
- https://wiki.archlinux.org/title/Wayland#Xwayland

### Check if you are on a GNU-based system

Run:

```bash
ldd --version
```

Expected output:

```text
ldd (GNU libc) x.xx
```

## Out of luck?

If none of the fixes above work, the issue may be specific to your system.

For reference, the client is developed and tested on:

```yaml
OS: Void Linux x86_64
Kernel: Linux 6.18.33_1
DE: Xfce4 4.20
WM: Xfwm4 (X11)
CPU: AMD Ryzen 5 3600
GPU: AMD Radeon RX 6600
```

If your setup differs significantly, compatibility issues are possible.

In that case, you can try building from source and then run the client.

## Building

Compiles with GCC. Tested with GCC 14.2.1.

1. Make the script executable:

```bash
chmod +x ./build.sh
```

2. Run the script:

```bash
./build.sh
```

## Project layout

```text
unwin-client/     <- Parent directory.
├── cef/          <- Third-party Chromium Embedded Framework (CEF) files.
├── include/      <- Project headers.
├── resources/    <- User resources. Used for storing resource files.
├── scripts/      <- User scripts. Used for storing JS/CSS/HTML files.
├── src/          <- Project source code.
│   ├── my_cef/        <- C implementations of CEF handlers and interfaces.
│   ├── my_pthread/    <- Non-blocking X event handling.
│   ├── MyX/           <- Error handling and window creation wrappers for X.
│   └── main.c         <- Entry point. CEF, window, browser, and thread startup.
└── build.sh      <- GCC build script.
```

Download the full project from the [Releases](https://github.com/pseudoical/unwin-client/releases/) page. It includes all required CEF binaries.

## JavaScript, CSS, and HTML injection

1. Open `unwin-client/scripts/`.
2. Create a new `.js`, `.css`, or `.html` file and paste your code.
3. Run the client.

### Notes

- Press `Ctrl+R` to reload the page after making changes.
- Backticks (`` ` ``) inside `.css` or `.html` files will cause an error for those files.
- Userscript headers are not supported (`@namespace`, `@run-at`, etc.).
- JavaScript executes as early as possible.
  - Use `addEventListener("load", ...)` if you need to wait for the page to finish loading:
    - https://developer.mozilla.org/en-US/docs/Web/API/Window/load_event#examples
- Files that do not end in `.js`, `.css`, `.html`, or `.htm` are ignored.
- Files may be stored in subfolders.

## Resource swapping

1. Open `unwin-client/resources/`.
2. Place your replacement files in that directory.
3. Run the client.

### Notes

- Press `Ctrl+R` to reload the page after making changes.
- The filename must exactly match the resource being requested.
- Press `F12` to open Chromium DevTools and inspect network requests.
- Files may be stored in subfolders.

## Creating a desktop shortcut

Create a file named `unwin.desktop` with the following contents:

```text
[Desktop Entry]
Type=Application
Name=unwin
Path=/path/to/unwin-client
Exec=./unwin
```

Replace `/path/to/unwin-client` with the actual path to your installation.

Example:

```text
[Desktop Entry]
Type=Application
Name=unwin
Path=/home/pseudoical/Documents/unwin-client
Exec=./unwin
```

Launch the application by double-clicking the `.desktop` file.

If you see a warning such as "Untrusted application launcher," click "Mark As Secure and Launch."

### Passing Chromium flags

Chromium command-line switches can be passed through the `Exec=` entry.

Example:

```text
Exec=./unwin --enable-zero-copy --enable-gpu-rasterization
```

For a complete list of Chrome flags, see:
- https://peter.sh/experiments/chromium-command-line-switches/

## Keyboard shortcuts

Unwin supports most Chrome keyboard shortcuts.

### Common shortcuts

| Shortcut | Action                  |
|----------|-------------------------|
| `F11`    | Toggle fullscreen       |
| `F12`    | Open Chromium DevTools  |
| `Ctrl+R` | Reload the current page |

For a complete list of Chrome keyboard shortcuts, see:
- https://support.google.com/chrome/answer/157179

## Known bugs

### Scrolling in menus

Scrolling may not work correctly inside some menu screens.

#### Workarounds

- Hold the middle mouse button while scrolling.
- Use the on-screen scroll bar.

Scrolling works normally while in-game.

## Third-party libraries

### Chromium Embedded Framework (CEF)

- Repository: https://github.com/chromiumembedded/cef
- Downloads: https://cef-builds.spotifycdn.com/index.html#linux64

For additional information, see [cef/README.txt](cef/README.txt).

## Contact me

Have any questions or bugs to report?

### Discord

Feel free to send me a friend request or DM!

- Username: `pseudoical`
- Profile: https://discord.com/users/1408292932624060426

## Thanks for checking out unwin

Hey, developer here!

I made this project alone, and it's the first "big" project I've fully completed.

There may still be bugs, so feel free to contact me if you find any.

This project is released under the MIT License. See [LICENSE](LICENSE) for details.

If you reference this project, I'd appreciate credit, but it's not required.

Thanks in advance, and no worries if you don't.