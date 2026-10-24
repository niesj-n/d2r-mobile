# Inventory Sort / Deposit row

User request and supplied screenshot: move Sort next to Deposit and adjust
both widths. Sort goes on the left in the existing bottom-right action space.
Keep gold, pouch, item grid, button height, game click messages and other panels.

The old Sort occupied y808 above the grid; Deposit occupied x916/y1323. The
bottom strip spans x916..1186.84: two original full-width sprites cannot fit.
Both actions now use 87px-wide versions of the original four-state Small art,
retaining all 46px of height and the original 16px end caps; only the centre
shortens. In native units each is174x92 at .74 scale, x916 and1058, y1323.
Their visible gap is13.24 units and the right edge1186.76 stays inside the old
Deposit footprint. Both use the same point size and original style.

The runtime pouch pump previously forced Deposit back to x916. Its Android
position and generator share product_inventory_row.h, included in the overlay
fingerprint. Non-Android pouch output and the bank Sort button stay unchanged.
Generated original pixels live only in the private import, never Git or APK.

Verification: actual Sort -> Pouch -> WideStash generation with the user's
private inputs (row/hit bounds, gold/grid exclusions, unchanged bank/iOS), all
four generated sprite states and byte-identical caps, source-built signed APK
plus lint, manifest/fingerprint and complete matching import ZIP. Independent
review before wireless in-place update. Device appearance/clicks remain manual.
