import importlib.util
import json
import os
from pathlib import Path
import unittest
from unittest.mock import patch

MODULE = Path(__file__).resolve().parents[1] / 'tools/product_ui_widgets.py'

class ProductWidgets(unittest.TestCase):
    def test_non_android_and_hud_stay_unchanged(self):
        self.assertTrue(MODULE.exists(), 'native panel widget generator is missing')
        spec = importlib.util.spec_from_file_location('product_ui_widgets', MODULE)
        mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
        text = '{"children":[]}'
        for platform in ('', 'ios', 'mac'):
            with patch.dict(os.environ, DM_PLATFORM=platform, DM_ANDROID_PRODUCT='0'):
                self.assertEqual(mod.augment_panel(text, 'controller/bankexpansionlayouthd.json'), text)
        self.assertFalse(hasattr(mod, 'augment_hud'))
        for env in ({'DM_PLATFORM': 'android', 'DM_ANDROID_PRODUCT': '0'},
                    {'DM_PLATFORM': '', 'DM_ANDROID_PRODUCT': '1'}):
            with patch.dict(os.environ, env):
                result = mod.augment_panel(text, 'controller/bankexpansionlayouthd.json')
                self.assertIn('TouchProductSort', result)
                self.assertEqual(mod.augment_panel(text, 'controller/hudpanelhd.json'), text)

    def test_only_inventory_and_bank_panels_gain_sort(self):
        self.assertTrue(MODULE.exists(), 'native panel widget generator is missing')
        spec = importlib.util.spec_from_file_location('product_ui_widgets', MODULE)
        mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
        text = '{"name":"Panel","children":[]}'
        with patch.dict(os.environ, DM_PLATFORM='android'):
            for path in ('controller/console/switch/playerinventoryexpansionlayouthd.json',
                         'Controller/BankExpansionLayoutHD.json'):
                result = mod.augment_panel(text, path)
                self.assertEqual(result, mod.augment_panel(result, path))
                child = json.loads(result)['children'][0]
                self.assertEqual(child['name'], 'TouchProductSort')
                self.assertEqual(child['fields']['textString'], '@dmPanelSort')
                self.assertEqual(child['fields']['onClickMessage'], 'PanelManager:ClosePanel:TouchProductSort')
            self.assertEqual(mod.augment_panel(text, 'controller/vendorpanellayouthd.json'), text)
        with patch.dict(os.environ, DM_PLATFORM='ios', DM_ANDROID_PRODUCT='0'):
            self.assertEqual(mod.augment_panel(text, 'controller/bankexpansionlayouthd.json'), text)

if __name__ == '__main__': unittest.main()
