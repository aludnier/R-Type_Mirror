#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "TowerDefense.h"

namespace {
std::filesystem::path defaultGameConfig()
{
    return std::filesystem::path(ENGINE_MODULE_DIR) / "../config/game.json";
}

void printUsage(std::string_view program)
{
    std::cout << "USAGE: " << program
              << " [scene.json [assets.json [input.json [game.json]]]]\n"
              << "Defaults to the Tower Defense files in towerDefense/.\n";
}
} // namespace

int main(int argc, char **argv)
{
    if (argc == 2 &&
        (std::string_view(argv[1]) == "-h" ||
            std::string_view(argv[1]) == "--help")) {
        printUsage(argv[0]);
        return 0;
    }
    if (argc > 5) {
        printUsage(argv[0]);
        return 2;
    }

    try {
        std::filesystem::current_path(TOWER_DEFENSE_ROOT);
        const std::filesystem::path project = "towerDefense";
        TowerDefenseGame::TowerDefense game(
            argc >= 2 ? argv[1] : project / "scene.json",
            argc >= 3 ? argv[2] : project / "config/assets.json",
            argc >= 4 ? argv[3] : project / "config/input.json",
            argc >= 5 ? argv[4] : defaultGameConfig());
        game.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
