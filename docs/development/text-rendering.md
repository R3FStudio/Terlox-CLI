# How CLI text reaches the screen

[Docs home](../README.md) · [Application controls](../reference/cli-protocols.md)

A CLI app sends text and escape sequences through its PTY. For example, an app
can choose red, print one letter, choose green, and print the next. Terlox stores
the style with each screen cell.

## Where to look

| Part | What it does |
| --- | --- |
| `terminal/Sequences.cpp` | Collects escape sequences, including ones split between reads |
| `terminal/Extensions.cpp` | Handles palette changes and Terlox-specific application controls |
| `dependencies/libvterm/engine/pen.c` | Keeps the current text style and interprets SGR commands |
| `terminal/Screen.cpp` | Exposes cells and resolves default, indexed and RGB colors |
| `terminal/Scrollback.cpp` | Keeps full cells, including styles, when lines scroll away |
| `window/TerminalPaint.cpp` | Turns visible cells into colored fills and glyphs |
| `renderer/GlyphCache.cpp` | Draws each glyph and its underline into a transparent texture |
| `renderer/SceneItem.cpp` | Places those textures in the Qt GPU scene |

The engine and renderer are shared by Windows, Linux and macOS. Their PTY and
window-backdrop code differs. A Windows console app may also have its output
translated by ConPTY before Terlox receives it.

Windows sessions use the bundled Microsoft console host when it is available,
including sessions opened normally. This keeps terminal palette queries working
on systems whose built-in console host drops them. The backend creates, resizes
and closes each session through the same API; a development build without the
bundle falls back to the system API. `ConptyApi.cpp` loads the runtime and
`platform/windows/ConPty.cpp` owns the session.

Apps can query the default foreground and background before choosing their own
panel and animation colors. Linux and macOS use a direct PTY stream, without
the Windows console host translating the output. Palette replies, RGB cells,
opacity and synchronized drawing use the same screen model on all three OSes.

`tests/TerminalClientFixture.cpp` runs a real interactive child on each platform.
It reads replies through Windows console input or a raw POSIX terminal, then
draws a panel and changing colors on adjacent letters. `ProcessTests.cpp` checks
the round trip, capability environment, animation frames and panel opacity.
It runs the child directly and through Terlox's shell; on Linux and macOS it
also uses `/bin/sh` and installed Bash, Zsh and Fish shells. These tests run in
the native CI jobs, alongside the shared screen-model and GUI tests.

## A style belongs to the characters already written

The current style is often called the *pen*. Changing it affects the next
characters. It must not recolor earlier output. Moving the cursor and writing
over a character replaces that cell's text and style. Scrolling, resizing and
switching between main and alternate screens must keep the cell attributes.

Erasing a line keeps the current background color and opacity, while clearing
text decorations. Apps use this to fill panels without printing a space in every
cell. Both the screen-model and GUI tests check this path, including the rendered
pixels and alignment between neighboring cells.

SGR 0 restores the default style. SGR 22 clears bold and faint without clearing
the chosen colors. The 256-color cube uses xterm's values, including 95 as its
first nonzero level; replacing that with evenly spaced values changes CLI themes.

Faint text reduces foreground alpha. It is separate from an application's
explicit text opacity, so the two multiply. Conceal hides a glyph without
removing its text. Blink changes only glyph visibility, leaving the background
and stored contents alone. Double and curly underlines are drawn separately
from the font so they don't collapse into an ordinary underline.

## RGB and per-cell opacity

Standard RGB escape sequences choose red, green and blue. Terlox's OSC 777
extension adds `textOpacity` and `cellBackgroundOpacity` to the pen. These values
are stored in the cells, including history. They do not change the window's tint
opacity, which is a separate session control.

The bundled libvterm has small local changes for faint, per-cell alpha, colon
RGB syntax and the xterm palette. They are listed in
[the dependency notes](../../dependencies/README.md). When updating libvterm,
preserve those changes and run the protocol and GUI tests.

## Check the result

`tests/TerminalProtocolTests.cpp` checks different colors on adjacent letters,
palette values, style resets, and alpha surviving history and resize. GUI tests
check the resulting glyph colors and a real rendered frame. Process tests check
the transport with a child program, which catches sequences a console host may
rewrite or drop.

Use `python examples/text_styles.py` inside Terlox for a quick visual check.
Compare bold, italic, faint and the underline variants at the normal text size.
Also check applications that use the affected protocols.

`terminal/Progress.cpp` handles OSC 9;4 state and percentages.
`window/TerminalProgress.cpp` adds its small indicator to the frame and animates
the indeterminate state. `TerminalPaint.cpp` holds the previous rendered frame
while a synchronized update is pending, even when a cursor or focus timer asks
for a repaint. This keeps intermediate animation frames out of the scene.
