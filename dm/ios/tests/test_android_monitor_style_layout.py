"""Android-only style sliders retain original keys and native slider artwork."""
import importlib
import json
import os
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import touch_layout as layout
import layout_patch
import dm_i18n

def nodes(root):
    result={root.get('name'):root}
    for c in root.get('children',[]):result.update(nodes(c))
    return result

for voice in ('0','1'):
    os.environ.update(MOVIE_FFMPEG=voice,DM_PLATFORM='ios',DM_ANDROID_PRODUCT='0');layout=importlib.reload(layout)
    original=layout._CUSTOM_KEYS
    assert 'perf_font_size' not in original and 'perf_transparency' not in original
    os.environ['DM_ANDROID_PRODUCT']='1';layout=importlib.reload(layout)
    assert layout._CUSTOM_KEYS[-5:]==('touch_margin_left','touch_margin_right','perf_overlay','perf_font_size','perf_transparency')
    root=layout_patch.load_text(layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')[0]);widgets=nodes(root)
    for key,default in [('perf_font_size','9'),('perf_transparency','45%')]:
        row=widgets['TouchCustom_'+key]
        assert row['type']=='SettingsSliderWidget' and row['fields']['increment']==1
        assert row['fields']['indicatorFilepath']=='PauseMenu/HorizontalIndicator'
        assert widgets['TouchCustom_'+key+'_value']['fields']['text']==default
        label=next(r for r in dm_i18n.rows() if r[0]==layout._custom_key(key))
        assert all(label[1:])
    # Only the Android native-game string artifact gains the two new labels;
    # the shared host-drawn table and non-Android ui.json retain their bytes.
    for android in ('0','1'):
        with patch.dict(os.environ,DM_ANDROID_PRODUCT=android,DM_PLATFORM='ios'):
            with tempfile.TemporaryDirectory() as tmp:
                with patch.object(layout,'read_layout',return_value=('data/ui.json','[{"id":1,"Key":"Existing"}]')):
                    result,_=dm_i18n.build('',tmp,{})
                entries=json.loads(Path(next(iter(result.values()))).read_text(encoding='utf-8-sig'))
                keys={r['Key'] for r in entries}
                assert ('dmCustomPerfFontSize' in keys)==(android=='1')
                assert ('dmCustomPerfTransparency' in keys)==(android=='1')
            assert 'dmCustomPerfFontSize' not in dm_i18n.header()
            assert 'dmCustomPerfTransparency' not in dm_i18n.header()
print('PASS Android monitor style native sliders and localized labels with/without voice')
