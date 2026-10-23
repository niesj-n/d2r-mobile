"""Existing shared Rift strings must reach both Android resource locales."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location('rift_gen_strings', ROOT / 'dm/android/tools/gen_strings.py')
GEN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GEN)


class RiftStrings(unittest.TestCase):
    def test_shared_host_strings_and_formats(self):
        rows = [line.split('\t') for line in (ROOT / 'dm/i18n/ui_text.tsv').read_text(encoding='utf-8').splitlines()]
        expected = {row[0]: row for row in rows if row[0].startswith('dmRiftHud') or row[0] == 'dmRiftTierN'}
        self.assertEqual(len(expected), 15)
        with tempfile.TemporaryDirectory() as directory:
            GEN.generate(Path(directory))
            for folder, column in [('values', 2), ('values-zh', 1)]:
                resources = {node.attrib['name']: node for node in ET.parse(Path(directory) / folder / 'strings.xml').getroot()}
                for key, row in expected.items():
                    self.assertIn(key, resources)
                    self.assertEqual(resources[key].text, row[column])
                    if row[column].count('%d') > 1:
                        self.assertEqual(resources[key].attrib.get('formatted'), 'false')


if __name__ == '__main__':
    unittest.main()
