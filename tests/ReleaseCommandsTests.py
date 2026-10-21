"""Check release routing without building, changing files or contacting GitHub."""
import argparse
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from release import common, package, publish


def options(**changes):
    values = dict(qt_root=None, skip_build=False, check=True, dry_run=True,
                  app_signing_identity='', installer_signing_identity='',
                  repo=None, ref='v1.0.0', tag=None, platform='all', publish=False)
    return argparse.Namespace(**(values | changes))


class ReleaseCommandsTests(unittest.TestCase):
    def test_native_routing(self):
        for system, packager, smoke in [('Windows', 'Package-Windows.ps1', 'Smoke-Package.py'),
                                        ('Linux', 'Package-Linux.py', 'Smoke-Unix-Packages.py'),
                                        ('Darwin', 'Package-macOS.py', 'Smoke-Unix-Packages.py')]:
            with self.subTest(system=system), patch.object(package.platform, 'system', return_value=system), patch.object(package, 'run') as run:
                package.local(options())
                commands = [call.args[0] for call in run.call_args_list]
                self.assertTrue(any(f'scripts/{packager}' in command for command in commands))
                self.assertIn(f'scripts/{smoke}', commands[-1])
                self.assertTrue(all(call.kwargs['dry_run'] for call in run.call_args_list))

    def test_skip_build_and_qt_path(self):
        with patch.object(package.platform, 'system', return_value='Windows'), patch.object(package, 'run') as run:
            package.local(options(skip_build=True, check=False, qt_root='C:/Qt kit'))
            run.assert_called_once()
            self.assertEqual(run.call_args.args[0][-2:], ['-QtRoot', 'C:/Qt kit'])

    def test_all_platforms_dispatches_requested_ref(self):
        with patch.object(package, 'run') as run:
            package.all_platforms(options(repo='owner/terlox'))
            command = run.call_args.args[0]
            self.assertIn('build.yml', command)
            self.assertEqual(command[command.index('--ref') + 1], 'v1.0.0')
            self.assertIn('owner/terlox', command)

    def test_asset_selection(self):
        self.assertEqual(len(common.assets('all')), 5)
        self.assertEqual(len(common.assets('linux')), 2)
        self.assertEqual(len(common.assets('macos-arm64')), 1)
        self.assertTrue(common.assets('macos-arm64')[0].name.endswith('-macos-arm64.pkg'))
        self.assertTrue(common.assets('macos-x64')[0].name.endswith('-macos-x64.pkg'))

    def test_wrong_version_cannot_publish(self):
        with patch.object(publish, 'run') as run, self.assertRaises(SystemExit):
            publish.publish(options(tag='v999.0.0'))
        run.assert_not_called()

    def test_new_release_defaults_to_draft(self):
        with patch.object(publish, 'run') as run:
            publish.publish(options(platform='windows'))
            commands = [call.args[0] for call in run.call_args_list]
            self.assertIn('--draft', commands[0])
            self.assertIn('--verify-tag', commands[0])
            self.assertFalse(any('--draft=false' in command for command in commands))
            self.assertNotIn('--clobber', commands[-1])


if __name__ == '__main__':
    unittest.main()
