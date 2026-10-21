"""Standard 24-bit ANSI colors work without any Terlox-specific API."""
print('\033[?2026h', end='')
for y in range(12):
    for x in range(64):
        r, g, b = x * 255 // 63, y * 255 // 11, 255 - x * 255 // 63
        print(f'\033[48;2;{r};{g};{b}m ', end='')
    print('\033[0m')
print('\033[?2026l', end='', flush=True)
