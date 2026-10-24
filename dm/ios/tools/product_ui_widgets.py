"""Android game-rendered buttons. Contains no art or fonts from the game files.

augment_panel accepts a layouts-relative path (case insensitive, optional
global/ui/layouts/ prefix) and a complete root-widget JSON document. It is
idempotent and returns the original bytes on other platforms / other panels.
"""
import json
import os
from pathlib import Path
import re
import struct

ACTION_ART = 'Controller/Panel/zspouch/ActionSmall'

def inventory_row():
    header = Path(__file__).resolve().parents[2] / 'android/product_inventory_row.h'
    values = dict(re.findall(r'^#define DM_ANDROID_SORT_ROW_(\w+) ([0-9.]+)$', header.read_text(), re.M))
    return {key: float(value) for key, value in values.items()}

def inventory_action(name, key, x):
    row = inventory_row()
    child = _button(name, key, x, row['Y'])
    child['fields'].update(filename=ACTION_ART, pointSize=row['POINT_SIZE'])
    child['fields']['rect'].update(width=int(row['FRAME_WIDTH'])*2, height=92, scale=row['SCALE'])
    return child

def inventory_store():
    child = inventory_action('Store', 'strGoldDeposit', inventory_row()['STORE_X'])
    child['name'] = 'TouchStore'
    child['fields']['onClickMessage'] = 'PanelManager:ClosePanel:TouchStore'
    return child

def build_art(romfs, outdir):
    """Shorten the original four-state button's centre; keep its height/caps.
    Generated private pixels stay in the import overlay, never Git or the APK.
    """
    if not _android(): return {}
    import romfs_art
    from PIL import Image
    source = 'hd/global/ui/FrontEnd/HD/Final/FrontEnd_ButtonSmall.sprite'
    raw = romfs_art.read(romfs, source)
    frame_width = struct.unpack_from('<H', raw, 6)[0]
    frames = struct.unpack_from('<H', raw, 20)[0]
    image = romfs_art.sprite(romfs, source)
    target = int(inventory_row()['FRAME_WIDTH']); cap = 16
    if frames != 4 or image.height != 46 or target <= cap*2 or frame_width <= target:
        raise ValueError('unexpected original compact-button geometry')
    result = Image.new('RGBA', (target*frames, image.height))
    for i in range(frames):
        frame = image.crop((i*frame_width, 0, (i+1)*frame_width, image.height))
        result.paste(frame.crop((0,0,cap,image.height)), (i*target,0))
        centre = frame.crop((cap,0,frame_width-cap,image.height))
        result.paste(centre.resize((target-2*cap,image.height),Image.Resampling.LANCZOS),(i*target+cap,0))
        result.paste(frame.crop((frame_width-cap,0,frame_width,image.height)),((i+1)*target-cap,0))
    # Same measured SpA1 v31 RGBA format used by gen_romfs_overlay's HUD.
    w,h = result.size
    header = b'SpA1'+struct.pack('<HHIIIIII',31,target,w,h,0,frames,0,0)+struct.pack('<II',w*h*4,4)
    path = Path(outdir) / 'product_action_small.sprite'
    path.write_bytes(header+result.tobytes())
    return {'+data/hd/global/ui/'+ACTION_ART.lower()+'.sprite':str(path)}

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
    if inventory:
        child = inventory_action('Sort', 'dmPanelSort', inventory_row()['SORT_X'])
    else:
        child = _button('Sort', 'dmPanelSort', 1100, 1300)
        child['fields']['rect'].update(width=366, height=92, scale=.74)
        child['fields'].update(filename='FrontEnd/HD/Final/FrontEnd_ButtonSmall', pointSize=40)
    prefix = text[:pos].rstrip()
    if not prefix.endswith(('[', ',')): prefix += ','
    return prefix + '\n' + json.dumps(child, ensure_ascii=False, indent=1) + '\n' + text[pos:]
