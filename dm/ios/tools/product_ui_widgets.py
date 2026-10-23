"""Android game-rendered buttons. Contains no art or fonts from the game files.

augment_panel accepts a layouts-relative path (case insensitive, optional
global/ui/layouts/ prefix) and a complete root-widget JSON document. It is
idempotent and returns the original bytes on other platforms / other panels.
"""
import json
import os

def _android():
    return os.environ.get('DM_PLATFORM') == 'android' or os.environ.get('DM_ANDROID_PRODUCT') == '1'

def _button(name, key, x, y):
    fields = {
        'rect': {'x': round(x, 1), 'y': round(y, 1), 'width': 400, 'height': 112, 'scale': .8},
        'filename': 'FrontEnd/HD/Final/FrontEnd_ButtonMed',
        'textString': '@' + key, 'text/style': '$StyleFEButtonText',
        'pointSize': 36, 'textColor': '$LightButtonTextColor',
        'normalFrame': 0, 'pressedFrame': 1, 'hoveredFrame': 3, 'disabledFrame': 2,
        'sound': 'cursor_menu_select_hd',
        'onClickMessage': 'PanelManager:ClosePanel:TouchProduct' + name,
    }
    return {'type': 'ButtonWidget', 'name': 'TouchProduct' + name, 'fields': fields}

def augment_panel(text, path):
    if not _android(): return text
    path = path.replace('\\', '/').lower().split('/ui/layouts/')[-1]
    inventory = path in ('controller/console/switch/playerinventoryexpansionlayouthd.json',
                         'controller/playerinventoryexpansionlayouthd.json')
    bank = path == 'controller/bankexpansionlayouthd.json'
    if not (inventory or bank) or '"TouchProductSort"' in text: return text
    # Preserve all inherited-game layout text, comments and trailing commas.
    # Only append a child to the root's final children array (same convention
    # as touch_layout's runeword controls).
    pos = text.rstrip().rfind(']')
    if pos < 0 or not text.rstrip().endswith('}'):
        raise ValueError('product widgets require a root children array: ' + path)
    child = _button('Sort', 'dmPanelSort', 1210 if inventory else 1100, 1323 if inventory else 1300)
    if inventory:
        # The final inventory bottom row already contains Pouch (222..502),
        # gold (512..906) and Store (916..1187). Its grid ends at x1196 and
        # ConsoleRightPanelRect ends at x1408: use that separate right strip.
        # The same native Small art as Store is 366x92 game units. At exact
        # scale .5 it fits 1210..1393; 64-point text becomes 32-point on screen.
        fields = child['fields']
        fields['rect'].update(width=366, height=92, scale=.5)
        fields.update(filename='FrontEnd/HD/Final/FrontEnd_ButtonSmall', pointSize=64)
    prefix = text[:pos].rstrip()
    if not prefix.endswith(('[', ',')): prefix += ','
    return prefix + '\n' + json.dumps(child, ensure_ascii=False, indent=1) + '\n' + text[pos:]
