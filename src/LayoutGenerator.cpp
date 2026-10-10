#include "LayoutGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <random>
#include <utility>

namespace layoutmaker {

namespace {

constexpr std::size_t kindIndex(ObjectKind kind) {
    return static_cast<std::size_t>(kind);
}

struct Profile {
    float lengthMultiplier;
};

Profile profileFor(std::string const& rating, float sourceHazardRatio) {
    if (rating == "Easy") return {0.75f};
    if (rating == "Normal") return {0.90f};
    if (rating == "Hard") return {1.00f};
    if (rating == "Harder") return {1.10f};
    if (rating == "Insane") return {1.20f};
    if (rating == "Easy Demon") return {1.30f};
    if (rating == "Medium Demon") return {1.40f};
    if (rating == "Hard Demon") return {1.50f};
    if (rating == "Insane Demon") return {1.60f};
    if (rating == "Extreme Demon") return {1.70f};

    // Auto reads the source hazard share; hazards are never generated.
    if (sourceHazardRatio < 0.10f) return {0.90f};
    if (sourceHazardRatio < 0.24f) return {1.00f};
    if (sourceHazardRatio < 0.40f) return {1.10f};
    return {1.20f};
}

struct Prototype {
    int objectID = 0;
    std::size_t frequency = 0;
};

} // namespace

std::size_t LayoutStats::count(ObjectKind kind) const {
    return counts[kindIndex(kind)];
}

bool isExcluded(ObjectKind kind, Exclusions const& exclusions) {
    switch (kind) {
        case ObjectKind::Block: return exclusions.blocks;
        case ObjectKind::Hazard: return exclusions.hazards;
        case ObjectKind::Gamemode: return exclusions.gamemodes;
        case ObjectKind::Pickup: return exclusions.pickups;
        case ObjectKind::Trigger: return exclusions.triggers;
        case ObjectKind::Decoration: return exclusions.decoration;
        case ObjectKind::Other: return exclusions.other;
        case ObjectKind::Count: return true;
    }
    return true;
}

LayoutStats analyze(std::vector<LayoutObject> const& objects, Exclusions const& exclusions) {
    LayoutStats result;
    bool first = true;

    for (auto const& object : objects) {
        if (isExcluded(object.kind, exclusions)) continue;

        ++result.total;
        ++result.counts[kindIndex(object.kind)];
        auto left = object.hasBounds ? object.boundsLeft : object.x;
        auto right = object.hasBounds ? object.boundsRight : object.x;
        auto bottom = object.hasBounds ? object.boundsBottom : object.y;
        auto top = object.hasBounds ? object.boundsTop : object.y;
        if (first) {
            result.minX = left;
            result.maxX = right;
            result.minY = bottom;
            result.maxY = top;
            first = false;
        } else {
            result.minX = std::min(result.minX, left);
            result.maxX = std::max(result.maxX, right);
            result.minY = std::min(result.minY, bottom);
            result.maxY = std::max(result.maxY, top);
        }
    }

    return result;
}

std::string formatStats(LayoutStats const& stats, std::string const& rating) {
    auto width = stats.total ? stats.maxX - stats.minX : 0.f;
    auto height = stats.total ? stats.maxY - stats.minY : 0.f;

    std::string text = "<cy>AUTO LAYOUT MAKER SCAN</c>\n";
    text += "Included objects: " + std::to_string(stats.total) + "\n";
    text += "Blocks / slopes: " + std::to_string(stats.count(ObjectKind::Block)) + "\n";
    text += "Hazards: " + std::to_string(stats.count(ObjectKind::Hazard)) + "\n";
    text += "Gamemode portals: " + std::to_string(stats.count(ObjectKind::Gamemode)) + "\n";
    text += "Orbs / pads / collectibles: " + std::to_string(stats.count(ObjectKind::Pickup)) + "\n";
    text += "Triggers: " + std::to_string(stats.count(ObjectKind::Trigger)) + "\n";
    text += "Decoration: " + std::to_string(stats.count(ObjectKind::Decoration)) + "\n";
    text += "Other: " + std::to_string(stats.count(ObjectKind::Other)) + "\n";
    text += "Bounds: " + std::to_string(static_cast<int>(width)) + " x " + std::to_string(static_cast<int>(height));
    text += "\nProfile: " + rating;
    return text;
}

std::vector<LayoutObject> makeRemix(
    std::vector<LayoutObject> const& source,
    RemixOptions const& options,
    SpawnPoint spawn
) {
    constexpr float gridSize = 30.f;
    constexpr float groundTop = 0.f;
    constexpr float groundTileCenterY = groundTop + gridSize * 0.5f;
    constexpr float maximumSpawnRadius = gridSize * 10.f;

    auto stats = analyze(source, options.exclude);
    if (stats.total < 2) return {};
    if (!std::isfinite(spawn.x) || !std::isfinite(spawn.y) || spawn.x < 0.f) return {};

    // Learn frequencies from plain 30-by-30 solid blocks. Restricting the palette
    // to one grid tile keeps the generated walk surface level and gap-free.
    std::map<int, Prototype> byObjectID;
    for (auto const& object : source) {
        if (isExcluded(object.kind, options.exclude) || !object.safeBlockPrototype || object.objectID <= 0) continue;
        if (std::abs(object.baseWidth - gridSize) > 0.5f || std::abs(object.baseHeight - gridSize) > 0.5f) continue;

        auto [it, inserted] = byObjectID.try_emplace(object.objectID);
        auto& prototype = it->second;
        if (inserted) {
            prototype.objectID = object.objectID;
        }
        ++prototype.frequency;
    }

    if (byObjectID.empty()) return {};

    std::vector<Prototype> prototypes;
    std::vector<double> weights;
    prototypes.reserve(byObjectID.size());
    weights.reserve(byObjectID.size());
    for (auto const& entry : byObjectID) {
        auto const& prototype = entry.second;
        prototypes.push_back(prototype);
        weights.push_back(static_cast<double>(prototype.frequency));
    }

    auto gameplayObjects = stats.count(ObjectKind::Block)
        + stats.count(ObjectKind::Hazard)
        + stats.count(ObjectKind::Gamemode)
        + stats.count(ObjectKind::Pickup);
    auto hazardRatio = gameplayObjects == 0
        ? 0.f
        : static_cast<float>(stats.count(ObjectKind::Hazard)) / static_cast<float>(gameplayObjects);
    auto profile = profileFor(options.rating, hazardRatio);

    std::mt19937 rng(options.seed);
    std::discrete_distribution<std::size_t> choosePrototype(weights.begin(), weights.end());

    // Leave a short, unobstructed run-up before the one-block-high platform.
    // The first block must still remain within the user's 10-block radius.
    constexpr float leadIn = gridSize * 7.f;
    auto firstGridIndex = std::ceil((spawn.x + leadIn - gridSize * 0.5f) / gridSize);
    auto startCenterX = firstGridIndex * gridSize + gridSize * 0.5f;
    auto spawnDistanceX = startCenterX - spawn.x;
    auto spawnDistanceY = groundTileCenterY - spawn.y;
    if (startCenterX <= 0.f || spawnDistanceX * spawnDistanceX + spawnDistanceY * spawnDistanceY
        > maximumSpawnRadius * maximumSpawnRadius) return {};

    auto sourceWidth = std::max(gridSize, stats.maxX - stats.minX);
    // Keep the generated section short enough to stay close to the spawn and
    // within a typical mobile editor/gameplay view.
    auto sourceColumns = std::clamp(std::ceil(sourceWidth / gridSize), 8.f, 12.f);
    auto columns = static_cast<std::size_t>(std::clamp(
        std::ceil(sourceColumns * profile.lengthMultiplier),
        8.f,
        14.f
    ));

    std::vector<LayoutObject> result;
    result.reserve(columns);

    std::uniform_int_distribution<std::size_t> segmentLength(2, 4);
    std::uniform_int_distribution<std::size_t> gapLength(1, 2);
    std::size_t column = 0;
    while (column < columns) {
        auto segmentColumns = std::min(segmentLength(rng), columns - column);
        for (std::size_t offset = 0; offset < segmentColumns; ++offset) {
            auto const& prototype = prototypes[choosePrototype(rng)];
            LayoutObject generated;
            generated.objectID = prototype.objectID;
            generated.kind = ObjectKind::Block;
            generated.x = startCenterX + static_cast<float>(column + offset) * gridSize;
            generated.y = groundTileCenterY;
            generated.rotation = 0.f;
            generated.scaleX = 1.f;
            generated.scaleY = 1.f;
            generated.flipX = false;
            generated.flipY = false;
            generated.baseWidth = gridSize;
            generated.baseHeight = gridSize;
            generated.safeBlockPrototype = true;
            result.push_back(generated);
        }
        column += segmentColumns;
        if (column >= columns) break;

        // Leave one or two cells at base-ground height between each short
        // raised section. Keep at least one more platform tile after the gap.
        auto gap = std::min(gapLength(rng), columns - column - 1);
        column += gap;
    }

    // Every raised section is one block high, and the base ground remains
    // continuous below all gaps.
    return result;
}

} // namespace layoutmaker
