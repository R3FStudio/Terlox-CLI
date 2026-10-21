# Contributing to Terlox

Terlox is an open-source terminal and shell, licensed under
[Apache 2.0](LICENSE). Code, documentation, bug reports and native-platform
testing are welcome.

## Before you change code

Read the [code map](docs/development/code-map.md) to find the component that owns
the behavior. Use the [build guide](docs/development/building.md) to get a local
build running. For a larger change, open an issue first so the design can be
discussed before you spend time implementing it.

Keep a pull request focused on one problem. Explain what happened before, what
changes, and how you checked it. Include the OS and shell for terminal bugs.
For visual changes, include an actual capture rather than a mockup. Say when a
platform was not tested.

## Code and documentation

- Place changes in the existing component that owns the behavior.
- Keep platform code behind the existing platform interfaces.
- Preserve OS authentication and permission checks for administrator/root commands.
- Keep settings and command arguments separate from shell strings unless a native
  shell explicitly needs a command string.
- Add a regression test for a bug when it verifies behavior rather than repeating
  the implementation. Run the relevant existing checks.
- Write user docs as instructions someone can follow. Keep implementation details
  in the development guides. Update docs when behavior or packaging changes.
- Keep binaries, generated builds and SDKs out of commits.

The [release guide](docs/development/releases.md) explains version changes,
packaging, updates and which artifacts belong in a release.

## Copyright and contribution license

You keep the copyright in work you create. There is no copyright assignment and
no separate CLA to sign.

By intentionally submitting original work for inclusion in Terlox, you submit it
under the repository's Apache License 2.0, as described in its Section 5. That
lets the project and its users use and distribute the contribution under that
license. Do not attach a conflicting exception, a revocation condition or an
additional restriction to a contribution.

Keep existing copyright, attribution and license notices. If you include someone
else's code, fonts or other material, identify its source and license and retain
the required notices. Third-party components keep their own applicable licenses;
do not relabel them as original Apache-licensed Terlox code.

Apache 2.0 includes copyright and patent grants, notice requirements, and warranty
and liability terms. Contributions remain available under those terms.
Contributors retain copyright and can separately license their own work.
Redistribution and commercial use are permitted under the license's conditions.
The [license text](LICENSE) controls; the
[Apache licensing FAQ](https://www.apache.org/foundation/license-faq.html) provides context.

## Sign off your commits

Use the [Developer Certificate of Origin 1.1](https://developercertificate.org/)
to certify you have the right to submit the work:

```sh
git commit -s
```

Git adds a `Signed-off-by: Your Name <your-email>` line. Use an identity you can
stand behind; a working GitHub noreply email is fine. This is a certification of
origin, not a transfer of copyright or a cryptographic signature. Sign-offs are
public in Git history. Co-authors should sign off too.

The contribution workflow checks commit sign-offs automatically. If your current,
unpublished commit is missing its sign-off, use
`git commit --amend --no-edit -s` before submitting it.

## Working with others

Be specific, respectful and patient. Discuss the code rather than the person.
Report reproducible problems, keep private data out of logs, and credit other
people's work. An issue or pull request may need a smaller scope or more evidence
before it can be merged.
