# Keyboard and mouse

[Docs home](../README.md)

## Text and the window

Drag across terminal content to select text. The empty strip above the first
line moves the window; it has the same glass as the rest of the surface.
Drag an edge or corner to resize. Double-clicking the top strip does not maximize.

New terminal windows request keyboard focus and come to the front once when
launched. They can then go behind other apps and minimize normally. Activation
on Linux depends on the desktop compositor honoring the launch request.

Some full-screen CLI programs use the mouse themselves. Hold **Shift** while
dragging to select text instead, or **Shift + right-click** to open Terlox's menu.

## Copy and paste

| Platform | Select all | Copy | Paste |
| --- | --- | --- | --- |
| Windows | Ctrl+A | Ctrl+C with a selection | Ctrl+V |
| macOS | Command+A | Command+C | Command+V |
| Linux | Ctrl+Shift+A | Ctrl+Shift+C or Ctrl+Insert | Ctrl+Shift+V or Shift+Insert |

On Windows, Ctrl+C without a selection interrupts the command. On Linux, plain
Ctrl+A/C/V belong to the shell. On macOS, physical Control keys belong to the
shell, while Command handles clipboard actions. Ctrl+Shift+C/V remain available
as additional shortcuts; Ctrl+Alt+A/C/V explicitly sends raw control keys.
Bracketed paste is used when the running program requests it.

In the included Terlox shell, Backspace or Delete removes selected text from
the command being edited. Select all can also include the prompt and previous output;
those parts remain unchanged. Selecting only output or the prompt clears the
selection without deleting anything. Native shells and CLI editors keep their
own editing behavior.

## Sessions and navigation

| Keys | Action |
| --- | --- |
| Ctrl+Shift+T | New session using your default profile |
| Ctrl+Shift+W | Close the current session |
| Ctrl+Tab | Next session |
| Ctrl+Shift+Tab | Previous session |
| Ctrl+Shift+P | Open the right-click menu from the keyboard |
| F1 | About and help |
| Alt+F10 | Maximize or restore |
| Ctrl+Shift+M | Minimize |

Tabs appear when a second session is opened.

## Zoom and history

Use **Ctrl + mouse wheel**, **Ctrl+=**, **Ctrl+-**, or **Shift + plus/minus** to
resize terminal text. Right-click also has **−**, **+**, a percentage and a reset
arrow. The arrow returns to your saved baseline size. Temporary zoom ends when
the window closes.

Scroll the wheel or use the slim right scrollbar to revisit output. It appears
when there is retained history and stays hidden in alternate-screen programs.
**Shift+PageUp / PageDown** moves a page through history. Typing returns to live
output. Scrollback uses a 32 MiB RAM cache per session. Older lines are compressed
into private temporary files and loaded again when you scroll up. The configured
scrollback line count limits cached rows in RAM; it does not discard older history.
Setting it to zero explicitly disables scrollback. Disk usage grows with retained
output. The temporary history is removed when you clear or close the session.
A storage failure produces a warning; additional history cannot be guaranteed
if temporary-disk space is exhausted.
Use `terlox clear` or **Clear terminal** in the context menu to clear the display
and retained history for the current session.

On macOS, Qt displays Command/Option equivalents for window action shortcuts.
