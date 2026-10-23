# Skill / bottom HUD clearance

The user's screenshot shows the lowest skill over the mana orb and mana label after increasing right-control margins. The previous spatial checks omitted bottom HUD art/text and therefore missed the collision. Add Android-only layout clearance against the actual imported v4 HUD geometry:1320-high reference, horizontal base754..2016 around1434, vital baseline1142 and conservative text/shadow top1100. Keep an8dp scaled gap around gameplay skill buttons.

Prefer lifting the whole combat fan just enough, including portal/switch/utility anchors, while keeping the upper controls on screen. If compact headroom is insufficient, finish clearing only the affected lower skill buttons. Preserve glyph sizes, held timing, directions, circle cancellation and right-margin following. Native hit circles and rendered positions remain the same snapshot. Reuse existing materials and update wirelessly without a readiness question, as authorized.

Verify the exact phone aspect ratio and the screenshot's approximate72dp margin, plus0/28/56/96, both swap profiles and common phone/tablet sizes. Test taps at corrected lower-key coordinates. Correct two legacy test output buffers still sized200 instead of the actual235-float snapshot before rerunning the suite; this is a test harness correction, not a product JNI array change. Power optimization remains deferred.
