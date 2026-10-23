"""Driver management is a standalone Android Custom button, never a setting key."""
import importlib
import json
import os
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import touch_layout as layout
import dm_i18n

def widgets():
    raw, _ = layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')
    page = json.loads(re.sub(r",\s*([}\]])", r"\1", raw))
    found = {}
    def visit(node):
        if isinstance(node, dict):
            if "name" in node:
                assert node["name"] not in found
                found[node["name"]] = node
            for child in node.values():
                visit(child)
        elif isinstance(node, list):
            for child in node:
                visit(child)
    visit(page)
    return found

for voice in (False, True):
    if voice:
        os.environ["MOVIE_FFMPEG"] = "1"
    else:
        os.environ.pop("MOVIE_FFMPEG", None)
    for android in (False, True):
        if android:
            os.environ["DM_ANDROID_PRODUCT"] = "1"
        else:
            os.environ.pop("DM_ANDROID_PRODUCT", None)
        layout = importlib.reload(layout)
        nodes = widgets()
        assert not any("driver" in key for key in layout._CUSTOM_KEYS)
        if not android:
            assert "TouchDriverSettings" not in nodes
            assert layout._CUSTOM_H == layout._CUSTOM_TITLE_H + sum(layout._CUSTOM_ROW_H)
            continue
        driver = nodes["TouchDriverSettings"]
        assert driver["type"] == "ButtonWidget"
        fields = driver["fields"]
        assert fields["textString"] == "@dmCustomDriverSettings"
        assert fields["onClickMessage"] == "PanelManager:ClosePanel:TouchDriverSettings"
        assert fields["filename"] == "FrontEnd\\HD\\Final\\FrontEnd_ButtonMed"
        last_key = nodes["TouchCustom_perf_overlay_Text"]["fields"]["rect"]
        assert fields["rect"]["y"] >= last_key["y"] + last_key["height"]
        assert fields["rect"]["y"] + 100 <= nodes["TouchCustomRows"]["fields"]["rect"]["height"]
        assert layout._CUSTOM_KEYS[-3:] == ("touch_margin_left", "touch_margin_right", "perf_overlay")
row = next(row for row in dm_i18n.rows() if row[0] == "dmCustomDriverSettings")
assert all(row[1:])
print("PASS Android-only native driver manager button, scroll extent, unchanged setting keys and all localized labels")
