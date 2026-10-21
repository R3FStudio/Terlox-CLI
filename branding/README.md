# Terlox identity

The icon shows a rose prompt and cursor in a wide, dark terminal window,
with a rose rim matching the app. Small raster sizes use a thicker rim so the
window remains recognizable in file lists. The painter is the canonical
editable source; exported assets are committed so a normal build needs no
graphics software or regeneration step.

Rebuild `terlox_icon_builder`, then run:

```
terlox_icon_builder branding/icons
```

PNG exports cover 16–1024 px, the Windows ICO contains seven PNG resolutions,
and the macOS ICNS contains standard and Retina slots. Windows EXE resources
embed the ICO in the terminal, shell and setup program. Qt uses the same PNGs
for window/task-switcher icons; macOS bundles use ICNS and Linux installs the
PNG sizes in hicolor. The scalable Linux SVG uses the same geometry.
