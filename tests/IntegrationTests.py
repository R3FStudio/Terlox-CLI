"""Check platform-independent installation metadata without changing this OS."""
import configparser
import importlib.util
from pathlib import Path
import plistlib
import tempfile
import unittest
import struct

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('defaults', ROOT/'integration/linux/set-default.py')
defaults = importlib.util.module_from_spec(spec)
spec.loader.exec_module(defaults)

class IntegrationTests(unittest.TestCase):
    def test_preferences_preserve_other_entries_and_are_idempotent(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'config/xdg-terminals.list'
            defaults.prefer(path)
            self.assertEqual(path.read_text(), defaults.ENTRY+'\n')
            path.write_text('# preferences\nother.desktop\n'+defaults.ENTRY+'\n')
            defaults.prefer(path)
            expected = defaults.ENTRY+'\n# preferences\nother.desktop\n'
            self.assertEqual(path.read_text(), expected)
            defaults.prefer(path)
            self.assertEqual(path.read_text(), expected)

    def test_terminal_desktop_entry(self):
        config = configparser.ConfigParser(interpolation=None)
        config.read(ROOT/'integration/linux/app.terlox.terminal.desktop')
        entry = config['Desktop Entry']
        self.assertIn('TerminalEmulator', entry['Categories'].split(';'))
        self.assertEqual(entry['X-TerminalArgExec'], '-e')
        self.assertEqual(entry['X-TerminalArgDir'], '--working-directory')
        self.assertEqual(entry['Terminal'], 'false')

    def test_native_bundle_identity(self):
        with (ROOT/'integration/macos/Info.plist.in').open('rb') as source:
            info = plistlib.load(source)
        self.assertEqual(info['CFBundleIdentifier'], 'app.terlox.terminal')
        self.assertEqual(info['CFBundlePackageType'], 'APPL')
        self.assertNotIn('CFBundleURLTypes', info)  # No unsupported URL handlers.
        self.assertEqual(info['CFBundleIconFile'], 'terlox.icns')

    def test_cross_platform_icon_exports(self):
        icons = ROOT/'branding/icons'
        for size in [16,24,32,48,64,128,256,512,1024]:
            png = (icons/f'terlox-{size}.png').read_bytes()
            self.assertEqual(png[:8], b'\x89PNG\r\n\x1a\n')
            self.assertEqual(struct.unpack('>II', png[16:24]), (size,size))
            self.assertEqual(png[25], 6)  # RGBA, transparent squircle corners.
        ico = (icons/'terlox.ico').read_bytes()
        self.assertEqual(struct.unpack_from('<HHH',ico), (0,1,7))
        for index in range(7):
            width,height,_,_,planes,bits,length,offset = struct.unpack_from('<BBBBHHII',ico,6+16*index)
            self.assertEqual((planes,bits),(1,32))
            self.assertEqual(ico[offset:offset+8],b'\x89PNG\r\n\x1a\n')
            self.assertLessEqual(offset+length,len(ico))
        icns = (icons/'terlox.icns').read_bytes()
        self.assertEqual(icns[:4],b'icns')
        self.assertEqual(struct.unpack_from('>I',icns,4)[0],len(icns))
        offset,types = 8,set()
        while offset < len(icns):
            kind,length=struct.unpack_from('>4sI',icns,offset)
            self.assertGreaterEqual(length,8); types.add(kind); offset+=length
        self.assertEqual(offset,len(icns))
        self.assertTrue({b'ic07',b'ic08',b'ic09',b'ic10',b'ic11',b'ic12',b'ic13',b'ic14'} <= types)

if __name__ == '__main__':
    unittest.main()
