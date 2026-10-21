# Sessions and commands

[Docs home](../README.md)

For the full terminal command list and shell examples, see the
[Command guide](command-guide.md).

A **terminal** displays text and passes your input to a program. A **shell** is
the program that understands commands. Terlox is the terminal; PowerShell, CMD,
Bash and the included `terloxsh` are different shells it can run.

## Choose a shell

Right-click and open **New session**. The list contains shells found on your
computer, plus any profiles you added. A profile is just a saved choice of shell,
arguments and starting folder.

Use **PowerShell** for PowerShell scripts, modules and aliases. Use **Command
Prompt** for CMD scripts. On Linux or macOS, choose your usual Bash, Zsh or Fish
session. Their normal startup files provide aliases, functions and shell setup.

The built-in **Terlox shell** is a command interpreter. It supports basic
commands, pipes, redirection and a few native-shell conveniences. It is useful
for everyday commands, but does not implement the whole PowerShell or POSIX
shell language. Choose the actual shell for a complex script.

## Commands you already installed

Git, Python and other executables are found through **PATH**, the list of folders
your OS searches for commands. Sessions inherit their environment variables.
Windows also adds missing registered user/system PATH entries to new sessions.

An alias such as a function from your PowerShell profile belongs to PowerShell;
it is not a separate executable. Open the matching shell if that alias is missing.
Restart a session after installing a new tool or changing its environment.

Windows sessions start in `%SystemRoot%\System32` unless you choose another folder.
Linux and macOS use your home folder. `cd` changes the current session's folder.

## Open an administrator session

Normal sessions stay unelevated. Right-click to opt in:

- **Windows:** **Run as administrator → PowerShell** or **Command Prompt**.
  Windows asks for UAC consent, then opens a separate Terlox window.
- **Linux:** **Open root session (sudo)** opens another tab.
- **macOS:** **Open administrator session (sudo)** opens another tab.

On Unix, sudo asks for authentication inside that tab; the graphical app stays
under your normal account. The action is disabled if sudo is not installed.

In the included Windows shell, a standalone executable that declares it needs
administrator rights requests UAC when you run it. Other commands can use
`sudo --cmd command` or `sudo --powershell command`. Cancelling UAC runs no command.
Linux/macOS keep native sudo authentication and hidden password input inside
the terminal. [Administrator commands and passwords](administrator-commands.md)
explains the details, including native shell profiles and password feedback.

## Set your usual shell

Type this into Terlox, using the exact name shown under **New session**:

```text
/terlox default-shell "powershell"
```

It applies to new sessions. Use `/terlox profile-add` to save another executable
and its launch options.

## Open a program directly

Editors and launchers can start a program without first opening a shell:

```text
terlox --working-directory "path with spaces" -e program "argument with spaces"
```

Put Terlox options before `-e`. Everything after it belongs to the program.
Terlox preserves those arguments; it does not expand shell syntax there. To use
a pipeline, start the appropriate shell and pass the pipeline to that shell.

## About and help

Press **F1**, type **About Terlox**, or type **/info-terlox**. The in-terminal
page shows shortcuts, version, current session and system information.
Use **1–4** or **Tab** to switch pages, arrows to scroll, and **Q** or **Esc** to
return to your session.
