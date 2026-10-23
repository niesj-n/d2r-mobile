"""Actual Sort -> Pouch -> WideStash production transforms using private inputs.

Set DM_TEST_ROMFS and DM_GEN_ASSETS to the existing game/generated asset roots.
No game data is committed, no guest is started, and no saves are read.
"""
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
import layout_patch
import pouch_layout
import product_ui_widgets
import romfs_art
import touch_layout
import wide_stash

INV = 'controller/console/switch/playerinventoryexpansionlayouthd.json'
BANK = 'controller/bankexpansionlayouthd.json'


def named(root, name):
    if root.get('name') == name:
        return root
    for child in root.get('children', []):
        result = named(child, name)
        if result:
            return result
    return None


def bounds(widget, romfs):
    rect = widget['fields']['rect']
    # The game rounds scales to tenths. Use the same rounding boundary as HUD
    # generation and conservative full sprite dimensions for these buttons.
    raw_scale = rect.get('scale', 1)
    scale = max(raw_scale, round(raw_scale, 1))
    filename = widget['fields'].get('filename', '')
    if filename.endswith(('ButtonSmall', 'ButtonMed')):
        sprite = romfs_art.read(romfs, 'hd/global/ui/' + filename + '.sprite')
        _, frame_width, _, height = struct.unpack_from('<HHII', sprite, 4)
        native = (frame_width * 2, height * 2)
    else:
        native = (rect['width'], rect['height'])
    width = max(rect.get('width', native[0]), native[0]) * scale
    height = max(rect.get('height', native[1]), native[1]) * scale
    return rect['x'], rect['y'], rect['x'] + width, rect['y'] + height


def overlaps(a, b):
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


class ProductPipeline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.romfs = os.environ.get('DM_TEST_ROMFS')
        if not cls.romfs or not os.environ.get('DM_GEN_ASSETS'):
            raise unittest.SkipTest('set DM_TEST_ROMFS and DM_GEN_ASSETS for real layout pipeline')
        cls.temp = tempfile.TemporaryDirectory()
        with patch.dict(os.environ, DM_ANDROID_PRODUCT='1'):
            repl, _ = touch_layout.build_ingame(cls.romfs, cls.temp.name,
                spec={p: touch_layout.INGAME[p] for p in (INV, BANK)})
            extra, _ = pouch_layout.build(cls.romfs, cls.temp.name, repl)
            repl.update(extra)
            extra, _ = wide_stash.build(cls.romfs, cls.temp.name, repl)
            repl.update(extra)
        cls.final = {p: Path(repl[touch_layout.read_layout(cls.romfs, p)[0].lower()]).read_text(encoding='utf-8') for p in (INV, BANK)}

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def test_final_inventory_and_widened_bank_are_valid_layouts(self):
        for path, text in self.final.items():
            with self.subTest(path=path):
                root = layout_patch.load_text(text)
                sort = named(root, 'TouchProductSort')
                self.assertEqual(sort['type'], 'ButtonWidget')
                self.assertEqual(sort['fields']['onClickMessage'], 'PanelManager:ClosePanel:TouchProductSort')
                self.assertEqual(sum(c.get('name') == 'TouchProductSort' for c in root['children']), 1)

    def test_sort_matches_native_store_art_and_dimensions(self):
        inventory = layout_patch.load_text(self.final[INV])
        store = named(inventory, 'TouchStore')['fields']
        for path in (INV, BANK):
            sort = named(layout_patch.load_text(self.final[path]), 'TouchProductSort')['fields']
            self.assertEqual(sort['filename'], store['filename'])
            self.assertEqual(sort['rect']['width'], store['rect']['width'])
            self.assertEqual(sort['rect']['scale'], store['rect']['scale'])
            self.assertEqual(sort['rect']['height'], 92)
            self.assertEqual(sort['pointSize'], 40)

    def test_inventory_sort_has_exclusive_click_space(self):
        root = layout_patch.load_text(self.final[INV])
        sort = bounds(named(root, 'TouchProductSort'), self.romfs)
        for name in ('TouchStore', 'TouchPouch'):
            with self.subTest(control=name):
                self.assertFalse(overlaps(sort, bounds(named(root, name), self.romfs)), name + ' must remain independently clickable')
        self.assertEqual(named(root, 'TouchStore')['fields']['rect'],
                         {'x': 916, 'y': 1323, 'width': 366, 'scale': .74})
        # Original inventory grid (98-unit cells), gold backing and panel edge
        # measured from the actual 3.3.94012 controller inheritance/profile.
        for name, rect in (('grid', (216, 893, 1196, 1285)),
                           ('gold frame', (512, 1294, 906, 1422)),
                           ('four-slot belt', (452, 750, 844, 848))):
            self.assertFalse(overlaps(sort, rect), name)
        profile = layout_patch.load_text(touch_layout.read_layout(self.romfs, 'controller/_profileconsolehd.json')[1])
        panel = profile['ConsoleRightPanelRect']
        self.assertGreaterEqual(sort[0], 0)
        self.assertGreaterEqual(sort[1], 0)
        self.assertLessEqual(sort[2], panel['width'])
        self.assertLessEqual(sort[3], panel['height'])
        # The native ButtonWidget keeps an exclusive interior click point in
        # the real panel, after the later-appended Pouch/Store layer is present.
        center = ((sort[0] + sort[2]) / 2, (sort[1] + sort[3]) / 2)
        for name in ('TouchStore', 'TouchPouch'):
            other = bounds(named(root, name), self.romfs)
            self.assertFalse(other[0] <= center[0] <= other[2] and other[1] <= center[1] <= other[3])
        for source in ('controller/playerinventoryoriginallayouthd.json', INV):
            inherited = layout_patch.load_text(touch_layout.read_layout(self.romfs, source)[1])
            for child in inherited['children']:
                rect = child.get('fields', {}).get('rect')
                if isinstance(rect, dict) and all(key in rect for key in ('x', 'y', 'width', 'height')):
                    other = (rect['x'], rect['y'], rect['x'] + rect['width'], rect['y'] + rect['height'])
                    self.assertFalse(overlaps(sort, other), child['name'])

    def test_bank_sort_does_not_cover_gold_paging_or_arrows(self):
        root = layout_patch.load_text(self.final[BANK])
        sort = bounds(named(root, 'TouchProductSort'), self.romfs)
        for name in ('gold_amount', 'gold_withdraw', 'StashNavigation', 'TouchStashLeft', 'TouchStashRight', 'TouchPgPage_Text'):
            rect = named(root, name)['fields']['rect']
            other = (rect['x'], rect['y'], rect['x'] + rect['width'], rect['y'] + rect['height'])
            self.assertFalse(overlaps(sort, other), name)
        self.assertLessEqual(sort[2], 1305 + wide_stash.EX)
        self.assertLessEqual(sort[3], 1421)

    def test_existing_comma_input_keeps_exact_legacy_pouch_output(self):
        with patch.dict(os.environ, DM_ANDROID_PRODUCT='0', DM_PLATFORM='ios'):
            with tempfile.TemporaryDirectory() as tmp:
                repl, _ = touch_layout.build_ingame(self.romfs, tmp, spec={INV: touch_layout.INGAME[INV]})
                text = Path(next(iter(repl.values()))).read_text(encoding='utf-8')
        pos = text.rstrip().rfind(']')
        legacy = text[:pos].rstrip(' ') + pouch_layout.widgets() + '    ' + text[pos:]
        self.assertEqual(pouch_layout.inv_layout(text), legacy)
        layout_patch.load_text(legacy)


class PouchChildSeparator(unittest.TestCase):
    def test_sort_then_pouch_is_parseable_without_private_inputs(self):
        with patch.dict(os.environ, DM_ANDROID_PRODUCT='1'):
            text = product_ui_widgets.augment_panel('{"children":[]}', INV)
        final = layout_patch.load_text(pouch_layout.inv_layout(text))
        self.assertIsNotNone(named(final, 'TouchProductSort'))
        self.assertIsNotNone(named(final, 'TouchStore'))

    def test_previous_trailing_comma_keeps_legacy_bytes(self):
        text = '{"children":[{"type":"Widget","name":"Existing"},\n    ]}'
        pos = text.rfind(']')
        legacy = text[:pos].rstrip(' ') + pouch_layout.widgets() + '    ' + text[pos:]
        self.assertEqual(pouch_layout.inv_layout(text), legacy)


if __name__ == '__main__':
    unittest.main()
