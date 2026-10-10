#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace layoutmaker {

enum class ObjectKind : std::uint8_t {
    Block,
    Hazard,
    Gamemode,
    Pickup,
    Trigger,
    Decoration,
    Other,
    Count
};

struct LayoutObject {
    int objectID = 0;
    ObjectKind kind = ObjectKind::Other;
    float x = 0.f;
    float y = 0.f;
    float rotation = 0.f;
    float scaleX = 1.f;
    float scaleY = 1.f;
    bool flipX = false;
    bool flipY = false;
    float baseWidth = 30.f;
    float baseHeight = 30.f;
    bool safeBlockPrototype = false;
    float boundsLeft = 0.f;
    float boundsRight = 0.f;
    float boundsBottom = 0.f;
    float boundsTop = 0.f;
    bool hasBounds = false;
};

struct Exclusions {
    bool gamemodes = false;
    bool blocks = false;
    bool hazards = false;
    bool pickups = false;
    bool triggers = true;
    bool decoration = false;
    bool other = false;
};

struct RemixOptions {
    std::string rating = "Auto";
    Exclusions exclude;
    std::uint32_t seed = 0;
};

struct SpawnPoint {
    float x = 0.f;
    float y = 105.f;
};

struct LayoutStats {
    std::array<std::size_t, static_cast<std::size_t>(ObjectKind::Count)> counts{};
    std::size_t total = 0;
    float minX = 0.f;
    float maxX = 0.f;
    float minY = 0.f;
    float maxY = 0.f;

    std::size_t count(ObjectKind kind) const;
};

bool isExcluded(ObjectKind kind, Exclusions const& exclusions);
LayoutStats analyze(std::vector<LayoutObject> const& objects, Exclusions const& exclusions);
std::string formatStats(LayoutStats const& stats, std::string const& rating);
std::vector<LayoutObject> makeRemix(
    std::vector<LayoutObject> const& source,
    RemixOptions const& options,
    SpawnPoint spawn
);

} // namespace layoutmaker
