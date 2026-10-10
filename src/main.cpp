#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/GeodeUI.hpp>

#include "LayoutGenerator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace layoutmaker {

ObjectKind classify(GameObject* object) {
    if (!object) return ObjectKind::Other;

    switch (object->getType()) {
        case GameObjectType::CubePortal:
        case GameObjectType::ShipPortal:
        case GameObjectType::BallPortal:
        case GameObjectType::UfoPortal:
        case GameObjectType::WavePortal:
        case GameObjectType::RobotPortal:
        case GameObjectType::SpiderPortal:
        case GameObjectType::SwingPortal:
        case GameObjectType::DualPortal:
        case GameObjectType::SoloPortal:
        case GameObjectType::NormalGravityPortal:
        case GameObjectType::InverseGravityPortal:
        case GameObjectType::NormalMirrorPortal:
        case GameObjectType::InverseMirrorPortal:
        case GameObjectType::RegularSizePortal:
        case GameObjectType::MiniSizePortal:
        case GameObjectType::TeleportPortal:
        case GameObjectType::GravityTogglePortal:
            return ObjectKind::Gamemode;

        case GameObjectType::Solid:
        case GameObjectType::Slope:
        case GameObjectType::Breakable:
        case GameObjectType::CollisionObject:
            return ObjectKind::Block;

        case GameObjectType::Hazard:
        case GameObjectType::AnimatedHazard:
            return ObjectKind::Hazard;

        case GameObjectType::YellowJumpPad:
        case GameObjectType::PinkJumpPad:
        case GameObjectType::GravityPad:
        case GameObjectType::RedJumpPad:
        case GameObjectType::SpiderPad:
        case GameObjectType::YellowJumpRing:
        case GameObjectType::PinkJumpRing:
        case GameObjectType::GravityRing:
        case GameObjectType::GreenRing:
        case GameObjectType::RedJumpRing:
        case GameObjectType::CustomRing:
        case GameObjectType::DashRing:
        case GameObjectType::GravityDashRing:
        case GameObjectType::SpiderOrb:
        case GameObjectType::TeleportOrb:
        case GameObjectType::Collectible:
        case GameObjectType::SecretCoin:
        case GameObjectType::UserCoin:
        case GameObjectType::DropRing:
            return ObjectKind::Pickup;

        case GameObjectType::Decoration:
            return ObjectKind::Decoration;

        default:
            break;
    }

    if (object->m_isDecoration || object->m_isDecoration2) return ObjectKind::Decoration;
    if (object->isTrigger()) return ObjectKind::Trigger;
    return ObjectKind::Other;
}

std::vector<LayoutObject> selectedObjects(EditorUI* ui) {
    std::vector<LayoutObject> result;
    if (!ui) return result;

    auto append = [&result](GameObject* object) {
        if (!object || object->m_isStartPos || object->m_isUIObject) return;

        LayoutObject item;
        item.objectID = object->m_objectID;
        item.kind = classify(object);
        auto position = object->getPosition();
        item.x = position.x;
        item.y = position.y;
        auto bounds = object->getBoundingRect();
        item.boundsLeft = bounds.origin.x;
        item.boundsRight = bounds.origin.x + bounds.size.width;
        item.boundsBottom = bounds.origin.y;
        item.boundsTop = bounds.origin.y + bounds.size.height;
        item.hasBounds = true;
        item.rotation = object->getRotation();
        item.scaleX = object->getScaleX();
        item.scaleY = object->getScaleY();
        item.flipX = object->isFlipX();
        item.flipY = object->isFlipY();
        auto objectSize = object->getContentSize();
        item.baseWidth = objectSize.width;
        item.baseHeight = objectSize.height;
        item.safeBlockPrototype = object->getType() == GameObjectType::Solid
            && !object->m_isDecoration
            && !object->m_isDecoration2
            && objectSize.width > 0.f
            && objectSize.height > 0.f
            && objectSize.width <= 120.f
            && objectSize.height <= 120.f;
        result.push_back(item);
    };

    if (ui->m_selectedObjects) {
        for (auto* object : CCArrayExt<GameObject, false>(ui->m_selectedObjects)) {
            append(object);
        }
    }

    if (ui->m_selectedObject && (!ui->m_selectedObjects || !ui->m_selectedObjects->containsObject(ui->m_selectedObject))) {
        append(ui->m_selectedObject);
    }

    return result;
}

Exclusions currentExclusions() {
    auto mod = Mod::get();
    return {
        .gamemodes = mod->getSettingValue<bool>("exclude-gamemodes"),
        .blocks = mod->getSettingValue<bool>("exclude-blocks"),
        .hazards = mod->getSettingValue<bool>("exclude-hazards"),
        .pickups = mod->getSettingValue<bool>("exclude-pickups"),
        .triggers = mod->getSettingValue<bool>("exclude-triggers"),
        .decoration = mod->getSettingValue<bool>("exclude-decoration"),
        .other = mod->getSettingValue<bool>("exclude-other")
    };
}

std::string currentRating() {
    return Mod::get()->getSettingValue<std::string>("rating");
}

bool supportsSafePlatformRoute(LevelSettingsObject* settings) {
    if (!settings) return false;
    // Start mode 0 is the standard cube. Other modes, mini/dual play, rotated,
    // flipped, mirrored, or platformer starts need different route physics.
    return settings->m_startMode == 0
        && !settings->m_startMini
        && !settings->m_startDual
        && !settings->m_twoPlayerMode
        && !settings->m_isFlipped
        && !settings->m_rotateGameplay
        && !settings->m_reverseGameplay
        && !settings->m_mirrorMode
        && !settings->m_platformerMode;
}

std::optional<SpawnPoint> playerSpawn(LevelEditorLayer* editor, std::string& error) {
    if (!editor) {
        error = "The editor is no longer available.";
        return std::nullopt;
    }

    auto* settings = editor->m_levelSettings;
    if (settings && settings->m_startsWithStartPos) {
        auto* start = editor->findStartPosObject();
        if (!start) {
            error = "Start Position mode is enabled, but no Start Position object was found.";
            return std::nullopt;
        }
        if (!supportsSafePlatformRoute(start->m_startSettings)) {
            error = "V2 currently supports a standard, normal-size, single-player cube start only. This keeps the one-block step and flat platform within known movement limits.";
            return std::nullopt;
        }
        auto position = start->getPosition();
        return SpawnPoint { position.x, position.y };
    }

    if (!supportsSafePlatformRoute(settings)) {
        error = "V2 currently supports a standard, normal-size, single-player cube start only. This keeps the one-block step and flat platform within known movement limits.";
        return std::nullopt;
    }

    if (settings && settings->m_spawnGroup > 0) {
        std::optional<SpawnPoint> found;
        if (editor->m_objects) {
            for (auto* object : CCArrayExt<GameObject, false>(editor->m_objects)) {
                if (!object || object->m_isStartPos || object->m_isUIObject) continue;
                for (short index = 0; index < object->m_groupCount; ++index) {
                    if (object->getGroupID(index) != settings->m_spawnGroup) continue;

                    auto position = object->getPosition();
                    SpawnPoint candidate { 0.f, position.y };
                    if (found && (std::abs(found->x - candidate.x) > 0.5f
                        || std::abs(found->y - candidate.y) > 0.5f)) {
                        error = "The configured Spawn Group has multiple possible positions, so V2 cannot place a guaranteed route.";
                        return std::nullopt;
                    }
                    found = candidate;
                    break;
                }
            }
        }
        if (!found) {
            error = "The configured Spawn Group has no matching object.";
            return std::nullopt;
        }
        return found;
    }

    // Geometry Dash's regular level entry point is at x=0, y=105.
    return SpawnPoint { 0.f, 105.f };
}

bool isStandardFloorTile(GameObject* object) {
    if (!object || object->getType() != GameObjectType::Solid
        || object->m_isDecoration || object->m_isDecoration2) return false;

    auto position = object->getPosition();
    auto size = object->getContentSize();
    auto bounds = object->getBoundingRect();
    auto boundsTop = bounds.origin.y + bounds.size.height;
    return std::abs(position.y - 15.f) <= 0.5f
        && std::abs(size.width - 30.f) <= 0.5f
        && std::abs(size.height - 30.f) <= 0.5f
        && std::abs(bounds.origin.y) <= 0.5f
        && std::abs(boundsTop - 30.f) <= 0.5f
        && std::abs(object->getRotation()) <= 0.5f
        && std::abs(object->getScaleX() - 1.f) <= 0.01f
        && std::abs(object->getScaleY() - 1.f) <= 0.01f;
}

bool hasClearGroundRoute(LevelEditorLayer* editor, SpawnPoint spawn, float endX) {
    if (!editor || !editor->m_objects) return true;

    constexpr float groundTop = 0.f;
    constexpr float tileSize = 30.f;
    constexpr float playerClearance = 120.f;
    auto left = std::max(0.f, spawn.x - tileSize * 0.5f);
    auto right = endX + tileSize * 0.5f;
    auto bottom = groundTop;
    auto top = groundTop + playerClearance;

    for (auto* object : CCArrayExt<GameObject, false>(editor->m_objects)) {
        if (!object || object->m_isStartPos || object->m_isUIObject
            || object->m_isDecoration || object->m_isDecoration2) continue;
        // Existing ordinary floor tiles support the same route and are safe.
        if (isStandardFloorTile(object)) continue;

        auto bounds = object->getBoundingRect();
        auto objectRight = bounds.origin.x + bounds.size.width;
        auto objectTop = bounds.origin.y + bounds.size.height;
        auto overlapsX = bounds.origin.x < right && objectRight > left;
        auto overlapsY = bounds.origin.y < top && objectTop > bottom;
        if (overlapsX && overlapsY) return false;
    }
    return true;
}

bool hasMatchingFloorTile(LevelEditorLayer* editor, float x, float y) {
    if (!editor || !editor->m_objects) return false;
    for (auto* object : CCArrayExt<GameObject, false>(editor->m_objects)) {
        if (!object || object->getType() != GameObjectType::Solid
            || object->m_isDecoration || object->m_isDecoration2) continue;
        auto position = object->getPosition();
        auto size = object->getContentSize();
        if (std::abs(position.x - x) <= 0.5f && std::abs(position.y - y) <= 0.5f
            && std::abs(size.width - 30.f) <= 0.5f && std::abs(size.height - 30.f) <= 0.5f
            && std::abs(object->getRotation()) <= 0.5f
            && std::abs(object->getScaleX() - 1.f) <= 0.01f
            && std::abs(object->getScaleY() - 1.f) <= 0.01f) return true;
    }
    return false;
}

void showMessage(std::string const& title, std::string const& message) {
    FLAlertLayer::create(title.c_str(), message.c_str(), "OK")->show();
}

void showScan(EditorUI* ui) {
    auto objects = selectedObjects(ui);
    if (objects.empty()) {
        showMessage("Auto Layout Maker", "Select the layout objects you want to scan, then press <cy>SCAN</c> again.");
        return;
    }

    auto stats = analyze(objects, currentExclusions());
    if (stats.total == 0) {
        showMessage("Auto Layout Maker", "Your current filters exclude every selected object. Open <cy>SET</c> to change them.");
        return;
    }
    showMessage("Auto Layout Maker", formatStats(stats, currentRating()));
}

void generateRemix(EditorUI* ui) {
    if (!ui || !ui->m_editorLayer) {
        showMessage("Auto Layout Maker", "The editor is no longer available. Reopen the editor and try again.");
        return;
    }

    auto objects = selectedObjects(ui);
    if (objects.size() < 2) {
        showMessage("Auto Layout Maker", "Select at least two layout objects before generating a V2.");
        return;
    }

    auto exclusions = currentExclusions();
    auto stats = analyze(objects, exclusions);
    if (stats.total < 2) {
        showMessage("Auto Layout Maker", "Fewer than two selected objects remain after filtering. Change your filters or selection.");
        return;
    }

    RemixOptions options;
    options.rating = currentRating();
    options.exclude = exclusions;
    options.seed = static_cast<std::uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );

    std::string spawnError;
    auto spawn = playerSpawn(ui->m_editorLayer, spawnError);
    if (!spawn) {
        showMessage("V2 could not start", spawnError);
        return;
    }
    auto remix = makeRemix(objects, options, *spawn);
    if (remix.empty()) {
        showMessage(
            "Auto Layout Maker",
            "V2 needs at least one ordinary 30 x 30 solid block in your selection. It leaves an open run-up, then builds short platform sections no more than one block high, with the base ground continuous below. The first block stays within 10 blocks of the player spawn."
        );
        return;
    }

    auto endX = remix.back().x;
    if (!hasClearGroundRoute(ui->m_editorLayer, *spawn, endX)) {
        showMessage(
            "V2 could not start",
            "Existing gameplay objects obstruct the spawn-to-platform corridor. Move them or clear that section first so V2 can keep the approach and platform clear."
        );
        return;
    }

    std::size_t created = 0;
    std::size_t alreadyPresent = 0;
    std::size_t failed = 0;
    for (auto const& item : remix) {
        if (hasMatchingFloorTile(ui->m_editorLayer, item.x, item.y)) {
            ++alreadyPresent;
            continue;
        }
        auto clone = ui->m_editorLayer->createObject(
            item.objectID,
            CCPoint { item.x, item.y },
            false
        );
        if (!clone) {
            ++failed;
            continue;
        }

        clone->setPosition({item.x, item.y});
        clone->setRotation(item.rotation);
        clone->setScaleX(item.scaleX);
        clone->setScaleY(item.scaleY);
        clone->setFlipX(item.flipX);
        clone->setFlipY(item.flipY);
        ++created;
    }

    if (created == 0) {
        if (failed == 0 && alreadyPresent == remix.size()) {
            showMessage("Auto Layout Maker", "A solid platform already exists at the generated positions; V2 did not add duplicate blocks.");
        } else {
            showMessage("Auto Layout Maker", "Geometry Dash could not create the platform blocks from this selection. No new blocks were added.");
        }
        return;
    }

    auto message = "Added " + std::to_string(created) + " platform blocks near the player spawn";
    if (alreadyPresent > 0) message += " and reused " + std::to_string(alreadyPresent) + " existing floor blocks";
    if (failed == 0) {
        message += ". The base-ground path stays continuous, and every raised section is at most one block high. The blocks are undoable.";
    } else {
        message += "; " + std::to_string(failed) + " blocks could not be created. The base ground remains passable, but use Undo if you want to remove this incomplete platform.";
    }
    message += "\n\nProfile: " + options.rating + " (section length). V2 adds solid blocks only; it does not add hazards, portals, orbs, triggers, or platforms higher than one block.";
    showMessage("V2 placed", message);
}

void confirmRemix() {
    geode::createQuickPopup(
        "Generate V2",
        "V2 rebuilds a short platform route near the player spawn using the selected layout's 30 x 30 solid-block frequencies. It leaves an open run-up, then builds short platform sections one block above continuous ground. The first block is within 10 blocks of spawn. Existing gameplay objects in the route stop generation. Your source stays in place, and new blocks are undoable.",
        "Cancel",
        "Generate",
        [](auto, bool generate) {
            if (!generate) return;
            generateRemix(EditorUI::get());
        }
    );
}

} // namespace layoutmaker

class $modify(LayoutMakerEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        if (!menu) return true;

        menu->setPosition({std::max(180.f, winSize.width * 0.14f), winSize.height * 0.74f});
        menu->setZOrder(1000);

        struct Action {
            char const* label;
            SEL_MenuHandler callback;
        };
        Action actions[] = {
            {"MAKE", menu_selector(LayoutMakerEditorUI::onLayoutMenu)},
            {"SET", menu_selector(LayoutMakerEditorUI::onLayoutSettings)},
            {"V2", menu_selector(LayoutMakerEditorUI::onLayoutRemix)},
            {"SCAN", menu_selector(LayoutMakerEditorUI::onLayoutScan)}
        };

        for (std::size_t index = 0; index < std::size(actions); ++index) {
            auto sprite = ButtonSprite::create(
                actions[index].label,
                "goldFont.fnt",
                "GJ_button_05.png",
                0.62f
            );
            if (!sprite) continue;

            auto button = CCMenuItemSpriteExtra::create(sprite, this, actions[index].callback);
            if (!button) continue;
            button->setPosition({0.f, -static_cast<float>(index) * 48.f});
            menu->addChild(button);
        }

        this->addChild(menu, 1000);
        return true;
    }

    void onLayoutMenu(CCObject*) {
        auto objects = layoutmaker::selectedObjects(this);
        if (objects.empty()) {
            layoutmaker::showMessage("Auto Layout Maker", "Select a section in the editor, then use <cy>SCAN</c> or <cy>V2</c>.");
            return;
        }

        auto stats = layoutmaker::analyze(objects, layoutmaker::currentExclusions());
        if (stats.total == 0) {
            layoutmaker::showMessage("Auto Layout Maker", "Your current filters exclude every selected object. Open <cy>SET</c> to change them.");
            return;
        }
        auto summary = layoutmaker::formatStats(stats, layoutmaker::currentRating());
        geode::createQuickPopup(
            "Auto Layout Maker",
            summary + "\n\nThe source stays in place. V2 uses its 30 x 30 solid-block frequencies to create a short platform near the player spawn. It leaves an open run-up, then builds short platform sections one block above continuous ground. Existing gameplay objects in the route stop generation.",
            "Settings",
            "Generate V2",
            [](auto, bool generate) {
                if (generate) layoutmaker::confirmRemix();
                else geode::openSettingsPopup(Mod::get(), true);
            }
        );
    }

    void onLayoutSettings(CCObject*) {
        if (!geode::openSettingsPopup(Mod::get(), true)) {
            layoutmaker::showMessage("Auto Layout Maker", "No preferences are available right now.");
        }
    }

    void onLayoutRemix(CCObject*) {
        layoutmaker::confirmRemix();
    }

    void onLayoutScan(CCObject*) {
        layoutmaker::showScan(this);
    }
};
