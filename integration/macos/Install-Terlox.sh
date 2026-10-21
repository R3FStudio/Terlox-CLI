#!/bin/sh
set -eu
bundle=${1:?Usage: sh Install-Terlox.sh /path/to/terlox.app}
test -x "$bundle/Contents/MacOS/terlox" || { echo 'Invalid Terlox application bundle' >&2; exit 1; }
mkdir -p "$HOME/Applications"
/usr/bin/ditto "$bundle" "$HOME/Applications/Terlox.app"
echo 'Installed in ~/Applications/Terlox.app. Open it from Finder or Spotlight.'
echo 'To select the terminal .command handler, run:'
echo '  ~/Applications/Terlox.app/Contents/MacOS/terloxctl default-terminal set'
echo 'Editors and Finder terminal services may have their own terminal preference.'
