"""Apply a transient CLI theme, opacity and scale, then restore preferences."""
from protocol import controls, theme, reset

try:
    controls(opacity=0.82, scale=1.2)
    theme('#bdf5eb', '#121828')
    print('\033[1;38;2;246;104;139mTERLOX / APPLICATION THEME\033[0m')
    print('This CLI app selected its colors, 1.2x text scale and 0.82 opacity.')
    input('Press Enter to restore your saved terminal appearance... ')
finally:
    reset()
