/*
** EPITECH PROJECT, 2026
** RType
** File description:
** SFML game asset module
*/

#include "Module.h"
#include "GameAssetTypes.h"

namespace {
RType::ModuleStatus registerSfmlAssets(AssetLoaderRegistry &assets)
{
    assets.registerType<GameTextureAsset>();
    assets.registerType<GameSoundAsset>();
    assets.registerType<GameMusicAsset>();
    assets.registerType<GameFontAsset>();
    return RType::ModuleStatus::Success;
}

RType::ModuleStatus registerSfmlModule(RType::Registry &)
{
    return RType::ModuleStatus::Success;
}
} // namespace

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    static const RType::ModuleDescriptor descriptor {
        .name = "sfml_assets",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerSfmlModule,
        .registerAssetTypes = registerSfmlAssets};
    return &descriptor;
}
