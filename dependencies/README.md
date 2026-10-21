# Vendored terminal engine

`libvterm/` is Paul Evans's MIT-licensed libvterm 0.3.3, obtained from
https://github.com/neovim/libvterm at commit
`934bc2fbf21800ac3458a499df8820ca5fb45fd3`.

The mirror is archived; upstream development is at
https://www.leonerd.org.uk/code/libvterm/ . Track upstream security and Unicode
updates before a public release. The library's license is preserved alongside it.

The implementation is in `libvterm/engine/`. The root CMake build compiles the
library using the included encoding tables. Upstream tests, documentation and
license files are retained alongside the code.

Terlox also carries these local changes in the MIT-licensed engine:

- SGR 2 faint intensity and its SGR 22 reset.
- Per-cell foreground/background alpha, set through
  `vterm_state_set_pen_alpha` by the OSC 777 extension. Alpha survives pen
  save/restore, history and resize. Erasing keeps the pen's background and alpha;
  SGR 0 restores both channels to 255.
- The xterm 256-color cube and grayscale values.
- Colon RGB sequences with an optional color-space field.

Changes are in `include/vterm.h`, `engine/vterm_internal.h`, `engine/pen.c`,
`engine/screen.c`, `engine/state.c` and `engine/vterm.c`. Preserve the upstream MIT notice when
carrying or rebasing them. The application's renderer handles the visual styles.

Qt 6.8 or newer is an external, dynamically linked build dependency. Qt is not
covered by Terlox's Apache-2.0 license. See `docs/Dependencies.md`.
