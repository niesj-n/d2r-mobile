# Quick controls v24

Approved scope: original jhaizhou-ops/d2r-mobile reviewed e3756f8; local base 83a3998.

The map visibility control is a bare original map icon at the safe top-left with configured left margin and 4*k edge spacing. Its 44dp phone hit target fits above the monitor's 48dp offset; keep it in that upper-left strip when possible. If scaled controls or panel bounds occupy that strip, place it beside the actual panel with top edges aligned and 10*k clearance; use the space below if the side does not fit. Native geometry owns both drawing coordinates and hit capture. The existing hidden DM_TC_MAP target retains its skill-cancel role.

Map position and item names continue the main skill outer arc, at a common 214dp scaled radius around MAIN, at -125 and -103 degrees. Their radius remains 18dp. Existing skills, I/II, portal, belt, queued actions and touch cancellation semantics remain unchanged. The 200-float snapshot retains all offsets and its first 185 floats.

PerformanceOverlay computes its panel once on size/insets/visibility changes, sends the same RectF used for drawing through DmActivity to TouchOverlay/native, and does not query or allocate each frame. Bounds changes cancel pending captures.
