"""Upload installers to GitHub. New releases stay drafts unless --publish is supplied."""
import argparse
from release.publish import publish


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', choices=['windows', 'linux', 'macos', 'macos-arm64', 'macos-x64', 'all'], default='all')
    parser.add_argument('--tag', help='Version tag; defaults to the project version')
    parser.add_argument('--repo', help='GitHub OWNER/REPO, otherwise use this checkout')
    parser.add_argument('--publish', action='store_true', help='Make the release public after uploading')
    parser.add_argument('--dry-run', action='store_true')
    publish(parser.parse_args())


if __name__ == '__main__':
    main()
