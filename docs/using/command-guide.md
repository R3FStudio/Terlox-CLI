# Command guide

[Docs home](../README.md)

Start here when you want to change a setting, run a command, or choose a shell.
Terlox includes its own shell, `terloxsh`. It also finds other shells installed
on your computer. Install additional shells separately.

| What you want to do | Read |
| --- | --- |
| Use the included shell's commands, variables and scripts | [Terlox shell](terlox-shell.md) |
| Use PowerShell, CMD, Bash, Zsh, Fish or sh | [Other shells](other-shells.md) |
| Change colors, fonts or transparency | Settings below; [Appearance](appearance.md) |
| Select the OS default terminal | Default-terminal commands below |
| Check for a new Terlox release | [Updates](updates.md) |
| Learn selection, zoom and keyboard shortcuts | [Keyboard and mouse](keyboard-and-mouse.md) |

## Terminal settings

Type these at a fresh prompt inside Terlox. They work across shell profiles
because the terminal handles them. They are interactive commands, not commands
to put in a Bash or PowerShell script.

| Command | What it does |
| --- | --- |
| `/terlox` | Show the saved appearance settings |
| `/terlox help` | List the terminal's commands |
| `/terlox border #79a7e8` | Change the active window border to a color you choose |
| `/terlox font "Cascadia Mono" 14` | Choose a font and text size |
| `/terlox opacity 0.45` | Change the background tint opacity |
| `/terlox blur 0.85` | Change the native background blur intensity |
| `/terlox effects on` or `/terlox effects off` | Enable or disable native material effects |
| `/terlox scrollback 10000` | Set how many old lines new sessions retain |
| `/terlox default-shell "name"` | Choose the shell for new sessions |
| `/terlox profile-add` | Add a shell executable, arguments and starting folder |
| `/terlox reset` | Restore the default appearance |
| `/terlox update` | Check for a stable release and confirm installation and restart; add `check` to only check |

Border colors are saved with your other settings. You can use a hex color such
as `#79a7e8` or a named color such as `tomato`. Inactive windows still use dark
gray; focusing the window restores your chosen color. `/terlox reset` restores
the default rose border along with the other appearance settings.

For `default-shell`, use the exact name under **Right-click > New session**.
Running `/terlox default-shell` without a name lists the available profiles.
For example, `/terlox default-shell "bash"` selects an installed Bash profile.

## About and help

Press **F1**, type **About Terlox**, or type `/info-terlox` to open the terminal's
About page. It includes shortcuts and details about your session. Use **Tab**
or **1-4** to change pages, arrows to scroll, and **Q** or **Esc** to leave.

## Default-terminal commands

These are ordinary executable commands. Run them from any installed shell:

| Command | What it does |
| --- | --- |
| `terloxctl default-terminal set` | Select Terlox for the OS integrations it supports |
| `terloxctl default-terminal status` | Show whether Terlox is selected |
| `terloxctl default-terminal restore` | Restore the preference saved before selecting Terlox |

The Terlox shell also has `terlox-default set`, `terlox-default status` and
`terlox-default restore`. The app executable accepts `--set-default-terminal`,
`--default-terminal-status` and `--restore-default-terminal` as alternatives.
See [Default terminal](default-terminal.md) for each OS's behavior.

## Administrator sessions

Use **Right-click > Run as administrator** on Windows, then choose PowerShell
or CMD and approve UAC. On Linux/macOS, the menu offers a sudo shell.

Inside Terlox you can also type `/admin` for an administrator session, or
`/admin --help` for its options. Windows accepts `/admin --powershell` and
`/admin --cmd`. An optional command after the options runs in that session.
The included shell offers `admin` without the slash too.

In the included Windows shell, standalone executables that require elevation
request UAC when you run them. `sudo --cmd command` and
`sudo --powershell command` request it explicitly for other commands.

Normal sessions stay under your usual account. On Linux/macOS, `sudo` and `su`
authenticate inside the terminal. Password input stays hidden; feedback marks
follow the system's sudo settings. See [Administrator commands and passwords](administrator-commands.md)
for the commands and each platform's behavior.

## Launch options

From an existing command line:

| Command | What it does |
| --- | --- |
| `terlox --help` | List the app's command-line options |
| `terlox --version` | Print the installed app version |
| `terlox --profile "name"` | Start a named shell profile |
| `terlox --working-directory "path"` | Start in a particular folder |
| `terlox -e program "argument"` | Run a program directly |
| `terlox --reset-text-size` | Reset saved text size and open Terlox |
| `terloxctl update` | Check for an update and confirm installation inside Terlox; add `check` to only check |

Place Terlox's options before `-e`; everything after it belongs to the program.
Use a shell with `-e` if the command needs pipes or shell syntax. Commands must
be on PATH, or you can use their full installed paths.
