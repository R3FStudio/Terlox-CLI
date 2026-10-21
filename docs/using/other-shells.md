# PowerShell, CMD and Unix shells

[Command guide](command-guide.md) · [Terlox shell](terlox-shell.md)

Open **Right-click > New session** and choose a shell. The list shows installed
shells Terlox can find, plus profiles you added. If a shell is missing, install
it or add its executable with `/terlox profile-add`.

Terlox supplies the window and terminal connection. Each shell supplies its own
language, commands, aliases and startup files. The examples below are commands
to type in the named shell.

## PowerShell

Choose `powershell` for Windows PowerShell or `pwsh` for an installed newer
PowerShell. On other platforms you can add an installed `pwsh` profile.

```powershell
Get-Location
Get-ChildItem
Set-Location "path with spaces"
$env:PROJECT = 'Terlox'
Get-Command *process*
Get-Help Get-ChildItem -Examples
```

Use `& '.\script.ps1'` to run a script in the current folder. Execution policy
and permissions still apply. PowerShell profiles provide your normal functions
and aliases. [PowerShell help](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/get-help)
explains how to find a command and its examples.

## Command Prompt on Windows

Choose `cmd` for batch files and CMD syntax:

```bat
cd
dir
cd /d "C:\path with spaces"
set PROJECT=Terlox
echo %PROJECT%
help
dir /?
```

Run a `.cmd` or `.bat` file by its path. Use `command /?` for help with a
particular Windows command. Microsoft's
[command reference](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/windows-commands)
lists the built-in commands and system tools.

## Bash and Zsh

Choose `bash` or `zsh` when available:

```sh
pwd
ls
cd "path with spaces"
export PROJECT=Terlox
echo "$PROJECT"
man ls
```

Run a Bash script with `bash script.sh`, or a Zsh script with `zsh script.zsh`.
Use `help cd` in Bash for a builtin; `man zshbuiltins` covers Zsh's builtins.
For the complete language, use the [Bash manual](https://www.gnu.org/software/bash/manual/html_node/index.html)
or [Zsh manual](https://zsh.sourceforge.io/Doc/Release/index.html).

On Windows, Bash comes from an installed Unix environment such as Git for
Windows. For WSL, add a profile using `wsl.exe` and its distribution arguments.
Commands and paths inside WSL belong to that Linux distribution.

## Fish

Fish has its own syntax, particularly for variables and scripting:

```fish
pwd
ls
cd "path with spaces"
set -gx PROJECT Terlox
echo $PROJECT
help set
```

Run Fish scripts with `fish script.fish`. Its
[tutorial](https://fishshell.com/docs/current/tutorial.html) covers interactive
use, variables, functions and scripting.

## sh

The `sh` profile runs your system's installed `sh`. Use POSIX shell syntax:

```sh
pwd
PROJECT=Terlox
export PROJECT
printf '%s\n' "$PROJECT"
sh script.sh
```

The implementation behind `sh` differs between systems. Check `man sh` for its
commands; Bash-only syntax may not work there.

## Which settings carry over?

Shell startup files load according to the profile's arguments. You can add a
profile with different arguments for a login shell or a particular configuration.
Changing `/terlox default-shell "name"` affects new sessions; it doesn't replace
the shell in an existing tab.

Terminal commands such as `/terlox border #79a7e8`, `/terlox update` and F1 work
at an interactive prompt across profiles. For scripts or other terminal apps,
use `terloxctl update` and `terloxctl default-terminal status` as ordinary
executables.
