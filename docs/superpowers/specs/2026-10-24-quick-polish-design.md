# Quick polish and loot-filter action

Approved scope: base44c7b1e, original jhaizhou-ops/d2r-mobile reviewed e3756f8942b40eba6c851e881828a5b1bda17fd6. The existing v2.4 screenshot shows sparse, low-contrast quick circles. The left-top bare map is already accepted and remains unchanged.

Map position and item filter reuse the exact skill button/socketArt bitmap path, 26dp scaled radius, silver-white bold text. Android resources provide complete labels; Chinese four-character labels split2+2, English labels split at the space, both fitted to the socket. Lines are cached once during view construction.

Default quick centers continue the skill/set-switch arc at186dp radius, -120/-138degrees relative to MAIN, with approximately58dp center spacing. When the original I/II is moved above-left, quick centers use the default I/II opening(-38,-156) and its neighboring arc position(-96,-170). All positions scale and translate with MAIN. Original skills, portal, I/II and belt do not move.

Action4 now means LOOT_FILTER, calling dm_custom_android_request_loot_filter_toggle() as an atomic request boundary. No legacy ITEM_NAMES enum alias is kept. The host worker implements the canonical setting/persistence pump. Action4 does not emitL or set labels_done; original automatic namesL and original name control/long-press paths remain intact. Snapshot offsets and200-float ABI, FIFO/capture/cancel, monitor bounds and left map stay unchanged.

Validation: red/green real bridge tests for filter requests and independent names, full-map/capture/scene cancellation; same five-size geometry matrix with actual2608x1200 density3 phone, margins0/96, both I/II positions, expanded four-row belt. Java21 resource/Java compile and NDK29 syntax. No device/renderer operations or overlay material regeneration.
