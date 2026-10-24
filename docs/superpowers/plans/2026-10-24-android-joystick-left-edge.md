# Floating joystick: left edge independent from the left button margin

User correction: the problem was the left-side start distance, not the upper or
right boundary. Left-margin adjustment must no longer affect the left joystick.

Actual Android queue evidence on869.333x400 with24dp left cutout/right margin72:
left slider0 starts atx0;24 starts at48;48 at72;72 at96;96 at120. A DOWN atx1
is ignored by the stick whenever the slider is nonzero. Movement deadzone is
not responsible; it is the shared layout's inherited start inset.

Pass zero left margin into the shared stick/combat layout. Retain margin_left
for left-side map shortcut placement. Do not patch only joy_zone_l: that would
leave shifted base clamping/right boundary and compact-layout margin coupling.
Retain right-margin control, safe visual edge clamps, floating held-only display,
original movement deadzone/travel, button/panel priority and sticky drag ownership.
Restore the shared40%-width/lower55% right/top start boundaries rather than the
previous misunderstood5% extension. No shared iOS source or material change.

Verify left-edge DOWN across all left slider values, zero DOWN/release axes,
all nine stick geometry values independent from left slider at fixed right margin,
left shortcut still moving, compact/phone layouts, skill geometry, repeated resize,
full-map drag and focus cancellation. Existing input/hold/ghost/frontend regressions,
signed source build/lint and independent review precede wireless in-place update.
Reuse v2.8.1 material; actual phone feel remains manual acceptance.
