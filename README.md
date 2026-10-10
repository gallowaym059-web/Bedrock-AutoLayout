# Auto Layout Maker

Auto Layout Maker is a Geode mod for Geometry Dash 2.2081. It adds a compact editor toolbar with **MAKE**, **SET**, **V2**, and **SCAN** actions. On mobile, the toolbar sits just to the right of the left control column and above the bottom build tray.

## Workflow

1. In the editor, select the objects that describe a layout section.
2. Press **SCAN** to see the selected object mix and bounds.
3. Set the rating profile and category exclusions from **SET** or the mod's Geode settings.
4. Press **V2** and confirm. The mod keeps the source in place and builds a short route near the player spawn.

V2 studies the selected section's ordinary 30 x 30 solid-block object-ID frequencies, then builds a new pattern from that weighted palette. It leaves about seven blocks of open ground ahead of the spawn, then places short, flat platform sections one block above the ground, separated by one- or two-block breaks. The first generated block must be within 10 blocks of the configured spawn. Existing ground stays continuous underneath, so the player can jump onto a section, return to ground through a break, and drop down safely at the end. It generates no hazards, portals, orbs, triggers, slopes, pickups, or unknown objects. Rating profiles change the section length only; they do not assign stars or gameplay difficulty. The total footprint is capped at 14 blocks to keep it close to the player and fit a typical mobile view.

V2 resolves the normal spawn, an active Start Position, or a single configured Spawn Group. It supports standard, normal-size, single-player cube starts only. It stops if the spawn is too far from the platform, or if gameplay objects obstruct the ground approach or platform clearance. Existing ordinary floor tiles are allowed. The selected source stays in place and generated objects are undoable.

The route is deliberately simple: a clear approach, one-block-high platform sections, and uninterrupted base ground below them. This avoids generated hazard patterns, high jumps, and ground gaps. It does not certify unrelated parts of the level.

## Build

Install the Geode SDK and CLI for your platform, set `GEODE_SDK` to the SDK directory, then run `geode build` from this folder.