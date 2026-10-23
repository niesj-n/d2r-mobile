"""Check default running policy and the optional selectable rewrite against 94012."""

import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

import lz4.block


TOOLS = Path(__file__).resolve().parent
FORCED_RUN_OFFSETS = (
    0x2c620c, 0x2c6210, 0x369a90, 0x2b30fc, 0x2b3104,
    0x6dc2c4, 0x6dcec0, 0x6c6580,
)
STAMINA_OFFSETS = (
    0x369980, 0x2b2c74, 0x2b2dc0, 0x6dce90,
    0x6c6648, 0x6c6290, 0x4ac368,
)


def original_image(path):
    data = path.read_bytes()
    flags = struct.unpack_from('<I', data, 0xc)[0]
    segments = [struct.unpack_from('<III', data, 0x10 + i * 0x10) for i in range(3)]
    compressed_sizes = struct.unpack_from('<III', data, 0x60)
    image = bytearray(segments[2][1] + segments[2][2])
    for i, (file_offset, memory_offset, size) in enumerate(segments):
        raw = data[file_offset:file_offset + compressed_sizes[i]]
        if flags >> i & 1:
            raw = lz4.block.decompress(raw, uncompressed_size=size)
        image[memory_offset:memory_offset + size] = raw
    return image


class WalkPolicyTest(unittest.TestCase):
    def test_android_import_restores_original_running_policy(self):
        from d2rm_android import patch_settings

        self.assertEqual(patch_settings('android', 'r44'), (
            'forced', 'r44-android',
            {'DM_TARGET': 'android', 'DM_RULESET': 'touch-run-v4', 'DM_ALLOW_WALK': '0'},
        ))
        self.assertEqual(patch_settings('ohos', 'r44'), (
            'forced', 'r44-ohos',
            {'DM_TARGET': 'ohos', 'DM_RULESET': 'touch-run-v4', 'DM_ALLOW_WALK': '0'},
        ))

    @unittest.skipUnless(os.environ.get('DM_WALK_TEST_MAIN'), 'set DM_WALK_TEST_MAIN to a 94012 main NSO')
    def test_selectable_policy_preserves_walk_and_private_gates(self):
        source = Path(os.environ['DM_WALK_TEST_MAIN'])
        original = original_image(source)
        env = os.environ.copy()
        env.update(DM_TARGET='android', DM_RULESET='touch-run-v4', DM_PRIVATE_GATES='1', DM_ALLOW_WALK='1')
        with tempfile.TemporaryDirectory(prefix='walk-policy-', dir=os.environ.get('DM_WALK_TEST_OUT')) as temp:
            walk_dir = Path(temp) / 'selectable'
            default_dir = Path(temp) / 'default'
            walk_dir.mkdir()
            default_dir.mkdir()
            proc = subprocess.run(
                [sys.executable, '-B', '-X', 'utf8', str(TOOLS / 'patch_nso.py'), str(source), str(walk_dir), 'main'],
                env=env, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=180,
            )
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            env.pop('DM_ALLOW_WALK')
            proc = subprocess.run(
                [sys.executable, '-B', '-X', 'utf8', str(TOOLS / 'patch_nso.py'), str(source), str(default_dir), 'main'],
                env=env, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=180,
            )
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            patched = (walk_dir / 'main.patched.bin').read_bytes()
            cuts = (walk_dir / 'main.cuts.tsv').read_text(encoding='utf-8')
            default_cuts = (default_dir / 'main.cuts.tsv').read_text(encoding='utf-8')
            cut_rows = {int(line.split('\t', 1)[0], 16): line for line in cuts.splitlines() if line and not line.startswith('#')}
            default_rows = {int(line.split('\t', 1)[0], 16): line for line in default_cuts.splitlines() if line and not line.startswith('#')}
            cut_offsets = set(cut_rows)
            for offset in FORCED_RUN_OFFSETS:
                with self.subTest(offset=hex(offset)):
                    self.assertEqual(patched[offset:offset + 4], original[offset:offset + 4])
                    self.assertNotIn(offset, cut_offsets)
                    self.assertIn(offset, default_rows)
            for offset in STAMINA_OFFSETS:
                with self.subTest(stamina_offset=hex(offset)):
                    self.assertIn(offset, cut_offsets)
                    self.assertNotEqual(patched[offset:offset + 4], original[offset:offset + 4])
            self.assertEqual(cut_rows, {offset: row for offset, row in default_rows.items()
                                        if offset not in FORCED_RUN_OFFSETS})
            for name in ('main.gates.tsv', 'main.obs.tsv', 'main.params.tsv',
                         'main.fixups.tsv', 'main.sites.tsv'):
                with self.subTest(table=name):
                    self.assertEqual((walk_dir / name).read_text(encoding='utf-8'),
                                     (default_dir / name).read_text(encoding='utf-8'))
            self.assertIn(0x2faf04, cut_offsets)  # private skill aim remains active
            self.assertTrue((walk_dir / 'main.aim.tsv').is_file())
            self.assertTrue((walk_dir / 'main.gates.tsv').stat().st_size)
            self.assertTrue((walk_dir / 'main.obs.tsv').stat().st_size)
            self.assertTrue((walk_dir / 'main.params.tsv').stat().st_size)


if __name__ == '__main__':
    unittest.main()
