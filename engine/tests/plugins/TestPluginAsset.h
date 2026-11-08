#ifndef TESTPLUGINASSET_H_
#define TESTPLUGINASSET_H_

#include <filesystem>
#include <string>
#include <string_view>

struct TestPluginAsset {
    static constexpr std::string_view JSON_TYPE = "TestPluginAsset";

    std::string path;

    static TestPluginAsset load(const std::filesystem::path &path)
    {
        return TestPluginAsset {path.string()};
    }
};

#endif
