# Check for updates

[Command guide](command-guide.md)

In the included Terlox shell, run:

```text
terlox-update
```

From PowerShell, CMD, Bash or another shell, use:

```text
terloxctl update
```

You can also type `/terlox update` at a fresh prompt inside the Terlox window.
`check` is an optional argument for the shell and `terloxctl` commands.

The checker contacts the
[Terlox GitHub repository](https://github.com/R3FStudio/Terlox-CLI), compares
the latest public stable release with the installed version, and shows the
release page. When an update is available, it includes matching download links
for your OS and architecture. Drafts and prereleases are excluded.

Nothing is installed automatically. Download the appropriate package, close
Terlox and run the installer. On Linux, choose the `.deb` or AppImage according
to how you installed the app. Replace an AppImage with the new file. See
[Installation](../getting-started/install.md) for the steps on each OS.

If there is no public release yet, the command says so. A private repository,
network problem or GitHub rate limit can also prevent a check. You can always
open the [Releases page](https://github.com/R3FStudio/Terlox-CLI/releases)
yourself. The checker doesn't require GitHub CLI or sign you into an account.
