# Appearance settings

[Docs home](../README.md)

Type `/terlox help` in the terminal for appearance commands. Type `/terlox` on its
own to see your current settings. These are Terlox commands, not shell settings.

## Font and text size

Cascadia Mono is included with the app, so you do not need to install the font.
It is the same family used by Windows Terminal. The default is 12 pt.

```text
/terlox font "Cascadia Mono" 14
```

That saves a new baseline. Ctrl+wheel and the menu zoom buttons are temporary;
they do not change the saved size. Supported text sizes are 6–40 pt.

You can name another installed font. Monospace fonts give each character the
same width, aligning text with the terminal grid.

## Glass and black tint

```text
/terlox opacity 0.45
/terlox blur 0.85
/terlox effects on
```

**Opacity** controls the black tint. A smaller number shows more of the windows
behind Terlox. The range is 0.2–1; the default is 0.45.

**Blur** controls the native material intensity from 0–1. The default is 0.85.
It is not a blur radius in pixels: the OS controls the material's blur kernel.
Set it to `0` to keep transparency without blur. **Effects** toggles native
material effects where the platform supports them.

Transparency depends on the desktop compositor. An application printing its own
background colors may cover parts of the glass; that is normal terminal output.

## Border and history

```text
/terlox border #ba5666
/terlox scrollback 10000
```

The outline defaults to rose. It turns dark gray while the window is inactive
and returns to your chosen color when you focus the window again.
Use `/terlox border #79a7e8` for a different color; the choice is saved.

Scrollback is the number of old lines retained by
new sessions; its range is 0–100000. Existing sessions keep their current history
allocation.

## Start over

```text
/terlox reset
```

This restores the default appearance. It does not remove your installed shells
or change OS preferences. Appearance settings are stored in your user account:
the Windows registry, macOS preferences or Linux user configuration through Qt.
