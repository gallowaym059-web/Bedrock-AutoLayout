# Auto Layout Maker

An in-editor Geometry Dash layout remix tool for Geode.

Select the objects that define a section, press **MAKE** to review the scan, then choose **V2** to build a new route near the player spawn. It learns ordinary 30 x 30 solid-block object-ID frequencies but does not reuse the source placements. Your original selection stays in place.

V2 uses a local procedural generator. It leaves an open run-up, then creates short flat platform sections one block above the ground line, with one- or two-block breaks between them. The first tile is within 10 blocks of the configured spawn, and the full pattern spans at most 14 blocks to fit a typical mobile view. Weighted block frequencies change the tile sequence; rating profiles change pattern length only. It never adds hazards, portals, orbs, triggers, slopes, or pickups. Base ground remains uninterrupted below the whole pattern.

V2 only supports a normal-size, single-player cube start, and rejects flipped gravity, rotated or reverse gameplay, mirrored or platformer starts, ambiguous Spawn Groups, and spawn points more than 10 blocks from the first tile. It also stops when gameplay objects obstruct the approach or platform clearance.
