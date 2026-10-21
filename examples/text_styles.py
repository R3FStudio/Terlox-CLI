"""Print ordinary ANSI styles and Terlox's per-letter opacity controls."""
import sys
from protocol import controls, reset


def write(text):
    sys.stdout.write(text)
    sys.stdout.flush()


try:
    write('\033[1mBold\033[22m  \033[2mFaint\033[22m  \033[3mItalic\033[23m\n')
    write('\033[4mSingle\033[24m  \033[4:2mDouble\033[24m  \033[4:3mCurly\033[24m  \033[9mStrike\033[29m\n')
    for color, letter in [((255, 95, 115), 'R'), ((95, 235, 160), 'G'), ((110, 160, 255), 'B')]:
        write(f'\033[38;2;{color[0]};{color[1]};{color[2]}m{letter}')
    write('\033[0m  Each letter has its own color.\n')
    for alpha in (1, .8, .6, .4, .2):
        controls(textOpacity=alpha)
        write(f'{alpha:.0%} text  ')
    write('\n')
finally:
    reset()
