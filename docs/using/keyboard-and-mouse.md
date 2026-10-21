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

| Keys | Action |
| --- | --- |
| Ctrl+A | Select retained terminal text, including scrollback |
| Ctrl+C | Copy selected text; without a selection, send an interrupt |
| Ctrl+V | Paste the clipboard |
| Ctrl+Shift+C / V | Alternative copy and paste shortcuts |
| Ctrl+Alt+A / C / V | Send the raw Ctrl key to the CLI program |

The raw-control shortcuts are useful when a CLI editor expects Ctrl+A or Ctrl+V
for its own commands. Terminal clipboard shortcuts otherwise take priority.

In the included Terlox shell, Backspace or Delete removes selected text from
the command being edited. Ctrl+A can also select the prompt and previous output;
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
output. Scrollback is limited by your configured history size.

On macOS, Qt displays Command/Option equivalents for actions in its shortcut
catalog. The terminal's text-input overrides use the Ctrl combinations listed
above; native macOS shortcut behavior still needs validation.
