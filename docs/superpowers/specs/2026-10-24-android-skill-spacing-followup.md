# Skill spacing follow-up

The user explicitly requests a larger rotating aim ring, more space between buttons, and the cancel target at the top right. Continuous held casting remains unchanged. The user has also authorized all future wireless updates without a save/exit confirmation.

Enlarge the visual aim ring by about one third and spread Android's four skill anchors by roughly 10–20 percent. Keep glyph/button sizes, aim-axis travel and held timing unchanged. Make room for the enlarged ring at the right screen edge. Draw and hit testing continue to use the same native geometry. Keep the utility pair aligned, move it left within the fan footprint and increase its pitch slightly so the top-right cancel circle has space.

The cancel circle prefers the actual screen's safe top-right corner, independently of right-control margins. It remains visible only during a skill gesture, uses the existing exact circle hit test and consumes a cancelled finger until release. Check actual phone and common tablet sizes, both set-switch profiles and 0/96 margins, native hold/cancel regressions, frontend dragging and the signed final APK. Reuse v2.5 import materials. Build, independently review, then install wirelessly without another readiness question.
