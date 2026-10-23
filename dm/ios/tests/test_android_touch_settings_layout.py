"""Android touch preferences append to Custom; other platforms retain their rows."""
import importlib
import json
import os
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import touch_layout as layout
import dm_i18n

added = ("touch_margin_left", "touch_margin_right", "perf_overlay")
for voice in (False, True):
    if voice:
        os.environ["MOVIE_FFMPEG"] = "1"
    else:
        os.environ.pop("MOVIE_FFMPEG", None)
    os.environ.pop("DM_ANDROID_PRODUCT", None)
    layout = importlib.reload(layout)
    original = layout._CUSTOM_KEYS
    assert not set(added).intersection(original)
    os.environ["DM_ANDROID_PRODUCT"] = "1"
    layout = importlib.reload(layout)
    assert layout._CUSTOM_KEYS[-3:] == added
    assert layout._CUSTOM_KEYS[:-3] == tuple(new for key in original for new in (("render_scale", "render_dynamic") if key == "q3d" else (key,)))
    raw, _ = layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')
    page = json.loads(re.sub(r",\s*([}\]])", r"\1", raw))
    nodes = {}
    def visit(node):
        if isinstance(node, dict):
            if "name" in node:
                assert node["name"] not in nodes
                nodes[node["name"]] = node
            for child in node.values():
                visit(child)
        elif isinstance(node, list):
            for child in node:
                visit(child)
    visit(page)
    for key in added[:2]:
        for direction in ("prev", "next"):
            name = f"TouchCustom_{key}_{direction}"
            assert nodes[name]["type"] == "ButtonWidget"
            assert nodes[name]["fields"]["onClickMessage"] == "PanelManager:ClosePanel:" + name
        assert nodes[f"TouchCustom_{key}_value"]["fields"]["text"] == "0"
        assert nodes[f"TouchCustom_{key}_Text"]["fields"]["text"] == "@" + layout._custom_key(key)
    assert nodes["TouchCustom_perf_overlay"]["type"] == "ToggleButtonWidget"
    assert nodes["TouchCustom_perf_overlay_Text"]["fields"]["text"] == "@dmCustomPerfOverlay"
    for key in added:
        row = next(r for r in dm_i18n.rows() if r[0] == layout._custom_key(key))
        assert all(row[1:])
    assert nodes["TouchCustomRows"]["fields"]["rect"]["height"] == layout._CUSTOM_H
print("PASS Android appended Custom margin selectors, monitor checkbox and localized labels, with/without voice")
