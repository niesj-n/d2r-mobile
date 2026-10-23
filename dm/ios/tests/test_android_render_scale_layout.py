"""Generated Android Custom page exposes exactly the fixed 3D scale choices."""
import importlib
import json
import os
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
os.environ["DM_ANDROID_PRODUCT"] = "1"
import touch_layout


def widgets():
    raw, _ = touch_layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')
    page = json.loads(re.sub(r",\s*([}\]])", r"\1", raw))
    found = {}
    def visit(item):
        if isinstance(item, dict):
            if "name" in item:
                assert item["name"] not in found
                found[item["name"]] = item
            for value in item.values():
                visit(value)
        elif isinstance(item, list):
            for value in item:
                visit(value)
    visit(page)
    return found


android = widgets()
for key in ("render_scale", "eco_fps"):
    for direction in ("prev", "next"):
        name = f"TouchCustom_{key}_{direction}"
        assert android[name]["type"] == "ButtonWidget"
        assert android[name]["fields"]["onClickMessage"].endswith(name)
    assert android[f"TouchCustom_{key}_value"]["type"] == "TextBoxWidget"
assert android["TouchCustom_render_scale_value"]["fields"]["text"] == "0.85×"
assert android["TouchCustom_eco_fps_value"]["fields"]["text"] == "60 FPS"
assert android["TouchCustom_render_dynamic"]["type"] == "ToggleButtonWidget"
assert "TouchCustom_q3d" not in android
assert not any(re.fullmatch(r"TouchCustom_render_scale_\d+", n) for n in android)

os.environ["MOVIE_FFMPEG"] = "1"
touch_layout = importlib.reload(touch_layout)
with_voice = widgets()
assert "TouchCustom_voice" in with_voice
assert "TouchCustom_render_scale_next" in with_voice
os.environ.pop("MOVIE_FFMPEG")

os.environ.pop("DM_ANDROID_PRODUCT")
touch_layout = importlib.reload(touch_layout)
other = widgets()
assert "TouchCustom_q3d" in other
assert not any(n.startswith("TouchCustom_render_scale_") for n in other)
assert "TouchCustom_eco_fps" in other
print("PASS Android fixed-scale choices and unchanged other-platform Custom page")
