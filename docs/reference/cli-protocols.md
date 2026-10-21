# CLI application support

[Docs home](../README.md)

This page is for people writing CLI applications. For ordinary shell use, start
with [Sessions and commands](../using/sessions-and-commands.md).

## Terminal identity and input

Sessions advertise `TERM=xterm-256color`, `COLORTERM=truecolor` and
`TERM_PROGRAM=Terlox`. An application should still query or handle missing
features rather than assume every xterm extension exists.

The screen engine supports alternate-screen programs, terminal key sequences,
bracketed paste, mouse reporting and focus reports. Hold Shift to override CLI
mouse ownership for terminal selection/menu actions. Terlox's copy/paste keys
take priority; Ctrl+Alt+A/C/V passes the corresponding raw Ctrl key through.

## Color and updates

Supported color controls include ANSI indexed colors, 256-color palettes,
24-bit foreground/background, OSC 4/104 palette operations, and OSC 10/11 with
110/111 default-color resets. Colors are stored per character. Apps can use
semicolon RGB (`38;2;R;G;B`) or colon RGB (`38:2::R:G:B`); use 48 for backgrounds.
The 256-color cube and grayscale ramp follow xterm's palette values.

Bold, faint, italic, single/double/curly underline, strike, conceal, reverse,
blink and superscript/subscript are rendered from each cell's attributes.
SGR 0 resets the style; SGR 22 clears bold and faint. Run the examples:

```sh
python examples/rgb_demo.py
python examples/text_styles.py
```

DEC synchronized updates (`CSI ? 2026 h/l`) allow an application to group output
into a visible update. A watchdog releases an abandoned update rather than
leaving the terminal stuck indefinitely.

Text animations use ordinary per-character colors and cursor rewrites; no
special shimmer API is needed. Pending synchronized frames stay hidden during
cursor and focus repaints as well as ordinary output updates.

## Progress

Carriage-return updates, cursor movement, erase-line commands and Unicode block
characters are rendered in the text grid. PowerShell's console progress display
is received through the native Windows console connection.

Terlox also accepts [OSC 9;4 progress notifications](https://learn.microsoft.com/en-us/windows/terminal/tutorials/progress-bar-sequences):

```text
ESC ] 9 ; 4 ; STATE ; PERCENT BEL
```

States are 0 (clear), 1 (normal), 2 (error), 3 (indeterminate) and 4 (paused).
Percentages are clamped to 0-100. These notifications show a small progress line
at the bottom of the terminal; the indeterminate state animates. Apps should
send state 0 when finished. This is separate from any progress text the app
draws itself.

## Temporary appearance controls

Terlox's OSC 777 extension accepts a Base64-encoded UTF-8 JSON object:

```text
ESC ] 777 ; terlox ; BASE64_JSON ESC \
```

| Field | Value | Effect |
| --- | --- | --- |
| `opacity` | Number, 0–1 | Temporary window tint opacity for this session |
| `scale` | Number, 0.5–3 | Text-size multiplier; effective size is clamped to 6–40 pt |
| `blur` | Number, 0–1 | Temporary native backdrop blur amount, where supported |
| `fontFamily` | Nonempty string, up to 128 characters | Temporary font family; missing fonts use Qt fallback |
| `borderColor` | Qt color string, such as `#ba5666` | Temporary window border color |
| `textOpacity` | Number, 0–1 | Foreground alpha for subsequently written characters |
| `cellBackgroundOpacity` | Number, 0–1 | Alpha for subsequently written explicit cell backgrounds |
| `reset` | `true` | Clear temporary appearance controls |

[examples/protocol.py](../../examples/protocol.py) provides Python helpers for
these sequences:

```python
from protocol import controls, reset

try:
    controls(opacity=0.6, scale=1.1, textOpacity=0.8)
    input("Press Enter to restore the appearance…")
finally:
    reset()
```

These controls do not write saved preferences or grant permissions. The JSON
payload is limited to 4096 bytes. Leaving the alternate screen resets transient
appearance controls. A main-screen application should reset explicitly: Terlox
cannot infer when every nested program in a shell has finished.

Text opacity changes the current pen, not all text already on screen. For
example, `controls(textOpacity=0.4)`, printing `A`, then
`controls(textOpacity=1)` and printing `B` leaves A translucent and B opaque.
It stays that way in scrollback. SGR 0 restores both pen alpha values to 1;
`reset()` restores them too. Faint halves foreground alpha on top of this value.
Selection highlighting takes priority over the app's cell background.

An app must emit these Terlox controls to request alpha or window changes;
ordinary ANSI RGB has no alpha channel. An existing app's theme continues to
use its own colors. Native window effects depend on the OS and compositor.

The ANSI behavior follows [xterm's SGR reference](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html).

## Images

The Windows example sends an iTerm2-style inline PNG using OSC 1337. On native
POSIX PTYs a limited Kitty direct-data transport supports PNG, RGB and RGBA,
chunked uploads, placement and deletion. File/shared-memory transports,
compression, animation, Sixel and the complete Kitty protocol are not implemented.

```sh
python examples/graphics_demo.py
```

Images are bounded: decoding rejects more than 4 million pixels or 24 MiB of
input, and storage/placements use additional memory/count limits. Older images
can be cleared when those limits are reached.

## Implementation and tests

### Editable command input

The included shell publishes its editable input using
`OSC 777;terlox-input;BASE64_JSON ST`. The decoded object contains `active`,
`text`, a `cursor` index in Unicode code points, and a `widths` array with one
cell width (0, 1 or 2) per code point. It is sent after drawing the command and
positioning its cursor. `{"active":false}` ends the editable region before
command execution, cancellation or EOF. The JSON limit is 64 KiB and the text
limit is 8192 UTF-16 units.

`terminal/EditableInput.cpp` maps selections to that region. The view sends Home,
Right and Delete keys to edit the selected characters in the shell's buffer.
Prompt text and output outside the region are excluded. An alternate screen,
Enter or Ctrl+C clears the region. A changed column count disables deletion
until the shell reports its updated layout. Partial selections across an
ambiguous wide-character wrap are left unchanged.

Applications that do not publish an editable region retain their own editing
behavior. Deleting a terminal selection outside a reported region clears only
the selection.

### Rendering and transport

[`terminal/Sequences.cpp`](../../terminal/Sequences.cpp) gathers fragmented escape
messages. [`Extensions.cpp`](../../terminal/Extensions.cpp) handles colors,
updates and application controls; [`Graphics.cpp`](../../terminal/Graphics.cpp)
handles images. [`window/ApplicationControls.cpp`](../../window/ApplicationControls.cpp)
validates and applies appearance changes. The renderer uploads resulting images
as textures.

`tests/TerminalProtocolTests.cpp` checks the protocol behavior, and
`tests/ProcessTests.cpp` checks native process output, palette replies, animation
frames and hidden input on each platform.
