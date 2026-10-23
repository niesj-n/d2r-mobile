"""Integration checks for the Android package's import source and app label."""

import os
from pathlib import Path
import subprocess
import sys
import unittest


PROJECT = Path(__file__).resolve().parents[1]
GRADLE = os.environ.get('DM_GRADLE', 'gradle')
BUILD_CONFIG = PROJECT / 'app/build/generated/source/buildConfig/release/com/xiaoboluo/d2r/BuildConfig.java'
MERGED_MANIFEST = PROJECT / 'app/build/intermediates/merged_manifest/release/processReleaseMainManifest/AndroidManifest.xml'


def gradle(*args):
    p = subprocess.run([GRADLE, '--no-daemon', '-p', str(PROJECT),
                        f'-PdmPython={sys.executable}', *args],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, encoding='utf-8', errors='replace', timeout=240)
    return p.returncode, p.stdout


class BuildIdentityTest(unittest.TestCase):
    def test_default_import_directory(self):
        code, output = gradle(':app:generateReleaseBuildConfig')
        self.assertEqual(code, 0, output[-3000:])
        self.assertIn('DM_IMPORT_DIRECTORY = "S2X/D2R Mobile"', BUILD_CONFIG.read_text(encoding='utf-8'))

    def test_separate_import_directory(self):
        code, output = gradle(':app:generateReleaseBuildConfig', '-PdmImportDirectory=S2X/D2R Niesj')
        self.assertEqual(code, 0, output[-3000:])
        self.assertIn('DM_IMPORT_DIRECTORY = "S2X/D2R Niesj"', BUILD_CONFIG.read_text(encoding='utf-8'))

    def test_rejects_unsafe_import_directory(self):
        code, output = gradle(':app:generateReleaseBuildConfig', '-PdmImportDirectory=../outside')
        self.assertNotEqual(code, 0, output[-3000:])
        self.assertIn('dmImportDirectory', output)

    def test_separate_app_label(self):
        code, output = gradle(':app:processReleaseMainManifest', '-PdmAppLabel=D2R Mobile 测试')
        self.assertEqual(code, 0, output[-3000:])
        self.assertIn('android:label="D2R Mobile 测试"', MERGED_MANIFEST.read_text(encoding='utf-8'))


if __name__ == '__main__':
    unittest.main()
