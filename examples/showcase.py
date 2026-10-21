"""A small real CLI program used in the README screenshot. No settings changed."""
import os
from pathlib import Path
import platform
import shutil

ROSE = '\033[38;2;225;145;165m'
WHITE = '\033[38;2;239;237;239m'
DIM = '\033[38;2;158;162;170m'
RESET = '\033[0m'


def main():
    project = Path(__file__).resolve().parents[1]
    print(f'\n  {ROSE}TERLOX{RESET}  {DIM}/  a little room for your command line{RESET}\n')
    print(f'  {WHITE}Native shells. Clear text. A quieter desktop.{RESET}\n')
    print(f'  {DIM}THIS SESSION{RESET}')
    print(f'  {WHITE}{platform.system()} {platform.machine()}{RESET}    {DIM}{os.environ.get("COLORTERM", "terminal colors")}{RESET}')
    shells = [name for name in ['pwsh', 'powershell', 'cmd', 'bash', 'zsh', 'fish'] if shutil.which(name)]
    print(f'  {DIM}Installed shells{RESET}   {WHITE}{" · ".join(shells) or "system shell"}{RESET}\n')
    print(f'  {DIM}INSIDE THE PROJECT{RESET}')
    for name, description in [('app', 'start the application'), ('window', 'tabs, selection and the terminal window'),
                              ('terminal', 'understand terminal output'), ('renderer', 'draw text and colors on the GPU'),
                              ('platform', 'connect to native shell processes')]:
        if (project / name).is_dir():
            print(f'  {ROSE}{name + "/":<14}{WHITE}{description}{RESET}')
    print(f'\n  {DIM}Ctrl+wheel{RESET} zoom    {DIM}Ctrl+Shift+T{RESET} new session    {DIM}F1{RESET} about\n')
    try:
        input(f'  {DIM}Press Enter to return{RESET} ')
    except EOFError:
        pass


if __name__ == '__main__':
    main()
