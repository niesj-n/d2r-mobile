# Android floating joystick presentation and larger start region

User request: do not show a fixed left stick; show it where a finger presses in
a lower-left region. Follow-up: expand that region slightly. Preserve held-base
stability, right skills, edge-distance settings, button priority and panel input.

The shared input already anchors each stick at DOWN and uses finger displacement
from DOWN for movement; edge clamps affect visual placement only. Android drew
its idle fallback base permanently, making this appear fixed. Draw it only while
the existing235-field owner flag is active; release/cancel hides it. Do not alter
the ABI or remove enabled idle geometry used by passive HUD avoidance.

On each freshly computed Android layout, extend the existing right boundary by
5% screen width, capped by the existing combat/margin limit. Raise the top from
45% to40% screen height. Without margin restrictions this is the left45% and
bottom60%. A gesture beginning outside cannot acquire the stick by sliding in;
an existing stick remains owned while dragging out. Shared iOS input is unchanged.

Verification: actual Android queue tests for newly admitted points, stationary
base, zero DOWN/release axes, full-map movement, panel/skill priority, margins,
focus cancellation and non-accumulating resize; existing touch/held-skill/drag
regressions. Source-built signed APK/lint and independent review before wireless
update. Existing v2.8.1 materials remain valid. Phone feel stays manual acceptance.
