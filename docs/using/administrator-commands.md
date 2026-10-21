# Administrator commands and passwords

[Docs home](../README.md) · [Command guide](command-guide.md)

You can run an elevated command without launching your everyday terminal as an
administrator. Authentication still belongs to the operating system.

## Linux and macOS

Use your installed `sudo` in the terminal, from Terlox's shell or a native shell:

```sh
sudo -v
sudo -- command argument
sudo -k
```

`sudo -v` authenticates or refreshes the system's cached authorization without
running another command. Replace `command argument` with the program you intend
to run. `sudo -k` invalidates the cached authorization. Your system decides which
account's password is required and which commands your account may run.

The prompt appears inside Terlox. Type the password and press Enter. Sudo
normally disables echo completely, so an empty-looking input line is expected.
If the system has password feedback enabled, its marks appear as usual. Terlox
does not print the password or add the response to shell history.

Password feedback is controlled by the system's **sudo policy**.
On systems using the sudoers policy, an administrator can use `sudo visudo` to
enable `Defaults pwfeedback`. This usually shows asterisks rather than dots.
Consult the installed `man sudoers` or the
[sudoers manual](https://www.sudo.ws/docs/man/sudoers.man/) before changing it.

`su`, `doas`, SSH password prompts and other installed tools keep their own
authentication behavior.
If sudo is absent or your account is not authorized, the terminal cannot grant
that permission; use your system's supported administration tools.

## Windows

In the included **Terlox shell**, typing a standalone executable whose manifest
requires administrator rights requests Windows UAC automatically. For example,
`diskpart` opens an elevated Terlox window after you approve the system prompt.
Your original terminal stays unelevated. The working folder and command are
passed to the elevated session.

For a command that needs elevated rights but does not declare it in its manifest,
request elevation explicitly:

```text
sudo --cmd command
sudo --powershell command
sudo -i
```

Replace `command` with your command or script. `sudo -i` opens an administrator
PowerShell session. The Windows `sudo` shortcut belongs to Terlox's shell and
uses the options listed by `sudo --help`.
Other shell profiles use their own installed commands, including Windows' native
sudo where it is available and enabled.

`/admin --cmd command` and `/admin --powershell command` work at a fresh prompt
in any Terlox GUI profile. The right-click administrator actions remain
available. These paths all use the Windows UAC dialog; passwords or administrator
credentials go into that system dialog. Cancelling it leaves the command unrun.

## Permission errors after a command has started

A program can perform some work and then encounter an access-denied error.
Terlox does not replay that program with extra rights: doing so could repeat
changes it already made. Inspect the error, then use `sudo` or `/admin` for the
operation you intend to authorize. Automatic Windows manifest elevation applies
to a standalone command; use an explicit elevated native command for pipelines,
redirection and scripts.
