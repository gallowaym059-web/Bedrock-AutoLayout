# Auto Layout Maker Changelog

## 1.0.3
- Leave an open run-up before short, flat one-block-high platform sections.
- Add one- or two-block breaks while keeping the base-ground route continuous.
- Keep the first generated block within 10 blocks of the configured player spawn.
- Cap the pattern footprint at 14 blocks to keep it near the player and fit a typical mobile view.
- Reject unsupported starts and obstructed gameplay corridors before generating.
- Never generate hazards, portals, orbs, triggers, slopes, pickups, or high jumps.
- Move the mobile toolbar clear of the editor's side controls and bottom build tray.

## 1.0.2
- Rebuild a new block pattern using the selected layout's solid-block frequency table.
- Keep generated blocks outside a continuous vertical corridor and omit hazards and interactive objects.
- Place V2 after current level content to avoid overlapping earlier generated passes.
- Show a clear message when filters exclude every selected object or usable block prototype.
- Report actual selected-object bounds and document the limits of playability checks.

## 1.0.1
- Place each V2 after the current level content to avoid overlapping earlier generated passes.
- Calculate Auto difficulty from gameplay objects instead of decoration and triggers.
- Show a clear message when filters exclude every selected object.
- Remove a platform-specific editor update call after object creation.

## 1.0.0
- Add an editor-side Layout Maker toolbar.
- Scan selected objects by category and bounds.
- Generate a varied second pass without replacing the source selection.
- Add rating profiles and category exclusion settings.
