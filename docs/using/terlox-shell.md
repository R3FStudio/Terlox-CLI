# Use the Terlox shell

[Command guide](command-guide.md) · [Other shells](other-shells.md)

Choose **Terlox shell** under **Right-click > New session**. It runs `terloxsh`,
the shell included with Terlox. Commands change the current session; opening
another session starts another shell.

## Included commands

| Command | Example | What it does |
| --- | --- | --- |
| `cd` | `cd "path with spaces"` | Change folders; `cd ..` goes up a level and `cd ~` goes home |
| `pwd` | `pwd` | Print the current folder |
| `echo` | `echo hello` | Print text and expanded variables |
| `clear` | `clear` | Clear the visible screen |
| `exit` | `exit` or `exit 0` | Leave the shell, optionally with an exit status |
| `export` | `export PROJECT=Terlox` | Set an environment variable for this session and its child programs |
| `set` | `set PROJECT=Terlox` | The same assignment form as `export` in this shell |
| `unset` | `unset PROJECT` | Remove an environment variable |
| `env` | `env` | List the environment; `export` or `set` with no arguments does this too |
| `history` | `history` | Show previous commands in an interactive session |
| `help` | `help` | List the included shell commands |
| `terlox-default` | `terlox-default status` | Check, set or restore the OS terminal preference |
| `terlox-update` | `terlox-update` | Check the latest public stable Terlox release |
| `/info-terlox` | `/info-terlox` | Show terminal information; inside the GUI it opens About |
| `admin` | `admin --help` | Show the options for an explicit administrator session |

On Windows, `cd` with no argument prints the current folder; on Unix it goes
home. Windows also accepts `cd /d`, `chdir`, `sl`, `Set-Location`, and drive
commands such as `D:`. Use quoted paths when a folder name contains spaces.

`set` here is not Bash's shell-option command or PowerShell's full variable
syntax. Choose those shells when following instructions written for them.

## Variables and quotes

```text
export PROJECT=Terlox
echo $PROJECT
echo "Working on ${PROJECT}"
echo '$PROJECT is literal here'
unset PROJECT
```

Single quotes preserve the text. Double quotes allow variable expansion.
`$NAME` and `${NAME}` read a variable, and `$?` reads the previous command's exit
status. `~` at the beginning of a path expands to your home folder.

Assignments use `export NAME=value` or `set NAME=value`. They last for the
session; put permanent environment settings in your OS or chosen shell's
configuration.

## Programs, pipes and files

Installed programs are found through PATH. For example, `git status` runs Git;
`npm install` runs your installed npm. Terlox does not provide those programs.

| Syntax | Meaning |
| --- | --- |
| `first; second` | Run commands in sequence |
| `first | second` | Pass the first program's output to the next |
| `program < input.txt` | Read input from a file |
| `program > output.txt` | Write output to a file, replacing its contents |
| `program >> output.txt` | Append output to a file |
| `program 2> errors.txt` | Write error output to a file |
| `# comment` | Comment at the start of a word, outside quotes |

For example, `echo hello > greeting.txt` creates a file in the current folder.
Some apps hide animations and progress bars when their output is piped or saved
to a file; use an ordinary interactive command when you want their live display.

This shell does not implement background jobs, `&&`, loops, functions or command
substitution. Use [Bash, PowerShell or another full shell](other-shells.md) for
scripts using those features.

## Scripts and one-off commands

Save Terlox commands in a text file and run it with:

```text
terloxsh tasks.tlx
terloxsh -c "echo hello"
```

`terloxsh --help` lists its launch options. A Terlox script is read line by line.
For a `.ps1`, `.cmd` or `.sh` script, use its actual interpreter instead.

## Editing a command

Use **Up/Down** for history, **Left/Right** to move, **Home/End** to reach either
end, **Backspace/Delete** to edit, and **Tab** for completion. **Ctrl+C** cancels
a command; with selected text, Terlox uses it to copy instead. **Ctrl+D** on an
empty prompt leaves the shell.

Ctrl+U clears the command being edited.

Select command text with the mouse and press Backspace or Delete to remove it.
Ctrl+A followed by either key clears the current command. The directory prompt
and previous output remain unchanged even when they are included in the selection.

The terminal owns its select/copy/paste shortcuts. **Ctrl+Alt+A/C/V** sends the
raw control key to the shell or app when needed. See
[Keyboard and mouse](keyboard-and-mouse.md) for the full list.
