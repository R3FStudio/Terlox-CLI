"""Small, dependency-free Terlox application protocol helpers."""
import base64
import json
import os
import sys

def controls(**values):
    payload = base64.b64encode(json.dumps(values).encode()).decode()
    sys.stdout.write(f'\033]777;terlox;{payload}\033\\')
    sys.stdout.flush()

def theme(foreground='#e8edf5', background='#101826'):
    sys.stdout.write(f'\033]10;{foreground}\033\\\033]11;{background}\033\\')
    sys.stdout.flush()

def reset():
    controls(reset=True)
    sys.stdout.write('\033]110\033\\\033]111\033\\\033]104\033\\\033[0m')
    sys.stdout.flush()

def png(data, columns=72, rows=14):
    encoded = base64.b64encode(data).decode()
    if os.name != 'nt' and 'kitty' in os.environ.get('TERLOX_GRAPHICS', ''):
        # Kitty transport chunks must stay <=4096 encoded bytes.
        for start in range(0, len(encoded), 4096):
            chunk = encoded[start:start + 4096]
            more = int(start + len(chunk) < len(encoded))
            header = f'a=T,f=100,c={columns},r={rows},q=2,m={more}' if start == 0 else f'm={more}'
            sys.stdout.write(f'\033_G{header};{chunk}\033\\')
    else:
        sys.stdout.write(f'\033]1337;File=inline=1;width={columns};height={rows}:{encoded}\033\\')
    sys.stdout.flush()
