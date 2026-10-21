"""One packaging entry point. Run --help for local and all-platform builds."""
import argparse
from release.package import local, all_platforms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--all', action='store_true', help='Build all platforms on GitHub Actions, wait, and download installers')
    parser.add_argument('--ref', default='main', help='Pushed branch or tag for --all (default: main)')
    parser.add_argument('--repo', help='GitHub OWNER/REPO, otherwise use this checkout')
    parser.add_argument('--qt-root', help='Qt kit directory for a local build')
    parser.add_argument('--skip-build', action='store_true', help='Package an existing local release build')
    parser.add_argument('--check', action='store_true', help='Run the native package smoke check afterward')
    parser.add_argument('--app-signing-identity', default='')
    parser.add_argument('--installer-signing-identity', default='')
    parser.add_argument('--dry-run', action='store_true', help='Print commands without running them')
    args = parser.parse_args()
    (all_platforms if args.all else local)(args)


if __name__ == '__main__':
    main()
