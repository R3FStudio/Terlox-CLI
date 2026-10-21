"""Upload the chosen platform's installers to a versioned GitHub release."""
import subprocess
from .common import assets, run, version


def publish(args):
    tag = args.tag or f'v{version()}'
    if tag != f'v{version()}':
        raise SystemExit('The tag must match the version in CMakeLists.txt.')
    files = assets(args.platform)
    missing = [p.name for p in files if not p.is_file()]
    if missing and not args.dry_run:
        raise SystemExit('Missing installers: ' + ', '.join(missing) + '. Run Package.py first.')
    repo = ['--repo', args.repo] if args.repo else []
    if args.dry_run:
        exists = False
    else:
        try:
            run(['gh', 'release', 'view', tag] + repo, capture=True)
            exists = True
        except subprocess.CalledProcessError:
            exists = False
    if not exists:
        run(['gh', 'release', 'create', tag, '--verify-tag', '--draft',
             '--title', tag, '--generate-notes'] + repo, dry_run=args.dry_run)
    run(['gh', 'release', 'upload', tag, *files] + repo, dry_run=args.dry_run)
    if args.publish:
        run(['gh', 'release', 'edit', tag, '--draft=false'] + repo, dry_run=args.dry_run)
    elif not args.dry_run:
        print('Installers uploaded. A new release stays a draft until you publish it.')
