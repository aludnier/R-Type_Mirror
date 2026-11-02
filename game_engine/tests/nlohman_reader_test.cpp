#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <iostream>

#include <nlohmann/json.hpp>

#include "library/NlohmanReader.h"

namespace {


struct WaveDef {
    float time;
    std::string enemy;
    int count;
};

struct LevelDef {
    int version;
    std::string name;
    std::vector<WaveDef> waves;
};

struct OptionalSettings {
    int port = 4242;
    bool autoFire = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WaveDef, time, enemy, count)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LevelDef, version, name, waves)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OptionalSettings, port, autoFire)


class TempFile {
public:
    explicit TempFile(const std::string &content)
    {
        static int counter = 0;
        _path = std::filesystem::temp_directory_path()
            / ("rtype_reader_test_" + std::to_string(counter++) + ".json");
        std::ofstream out(_path, std::ios::trunc);
        out << content;
    }

    ~TempFile()
    {
        std::error_code ec;
        std::filesystem::remove(_path, ec);
    }

    TempFile(const TempFile &) = delete;
    TempFile &operator=(const TempFile &) = delete;

    const std::filesystem::path &path() const { return _path; }

private:
    std::filesystem::path _path;
};

const char *const LEVEL_JSON = R"({
    "version": 1,
    "name": "level1",
    "port": 4242,
    "volume": 0.5,
    "waves": [
        { "time": 2.0, "enemy": "bydos_basic", "count": 5 },
        { "time": 10.0, "enemy": "bydos_shooter", "count": 3 }
    ]
})";

} // namespace

TEST_CASE("nlohamReader readFile loads a valid file", "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(file.path()));
    REQUIRE(reader["name"] == "level1");
}

TEST_CASE("nlohamReader readFile returns false for a missing file",
    "[config][json-reader]")
{
    RType::nlohamReader reader;
    const auto missing = std::filesystem::temp_directory_path()
        / "rtype_reader_does_not_exist.json";

    REQUIRE_FALSE(reader.readFile(missing));
}

TEST_CASE("nlohamReader readFile returns false for malformed JSON",
    "[config][json-reader]")
{
    TempFile file("{ \"version\": 1, \"name\": ");
    RType::nlohamReader reader;

    REQUIRE_FALSE(reader.readFile(file.path()));
}

TEST_CASE("nlohamReader readFile returns false for an empty file",
    "[config][json-reader]")
{
    TempFile file("");
    RType::nlohamReader reader;

    REQUIRE_FALSE(reader.readFile(file.path()));
}

TEST_CASE("nlohamReader failed readFile clears previously loaded data",
    "[config][json-reader]")
{
    TempFile good(LEVEL_JSON);
    TempFile bad("not json");
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(good.path()));
    REQUIRE_FALSE(reader.readFile(bad.path()));
    REQUIRE_THROWS_AS(reader["name"], nlohmann::json::out_of_range);
}

TEST_CASE("nlohamReader readFile replaces data on reload",
    "[config][json-reader]")
{
    TempFile first(R"({"name": "first"})");
    TempFile second(R"({"name": "second"})");
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(first.path()));
    REQUIRE(reader["name"] == "first");
    REQUIRE(reader.readFile(second.path()));
    REQUIRE(reader["name"] == "second");
}

TEST_CASE("nlohamReader constructor loads a valid file",
    "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);

    REQUIRE_NOTHROW(RType::nlohamReader(file.path()));
}

TEST_CASE("nlohamReader constructor throws for a missing file",
    "[config][json-reader]")
{
    const auto missing = std::filesystem::temp_directory_path()
        / "rtype_reader_does_not_exist.json";

    REQUIRE_THROWS_AS(RType::nlohamReader(missing), std::runtime_error);
}

TEST_CASE("nlohamReader constructor throws for malformed JSON",
    "[config][json-reader]")
{
    TempFile file("{ broken");

    REQUIRE_THROWS_AS(RType::nlohamReader(file.path()), std::runtime_error);
}

TEST_CASE("nlohamReader default constructed reader has no data",
    "[config][json-reader]")
{
    RType::nlohamReader reader;

    REQUIRE_THROWS_AS(reader["anything"], nlohmann::json::out_of_range);
}

TEST_CASE("nlohamReader operator[] throws for a missing key",
    "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE_THROWS_AS(reader["unknown"], nlohmann::json::out_of_range);
}

TEST_CASE("nlohamReader operator[] allows chained access",
    "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE(reader["waves"].size() == 2u);
    REQUIRE(reader["waves"].at(0).at("enemy") == "bydos_basic");
    REQUIRE(reader["waves"].at(1).at("count") == 3);
    REQUIRE_THROWS_AS(
        reader["waves"].at(0).at("missing"), nlohmann::json::out_of_range);
    REQUIRE_THROWS_AS(reader["waves"].at(5), nlohmann::json::out_of_range);
}

TEST_CASE("nlohamReader get returns typed values", "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.get<int>("port") == 4242);
    REQUIRE(reader.get<std::string>("name") == "level1");
    REQUIRE(reader.get<float>("volume") == 0.5f);
}

TEST_CASE("nlohamReader get throws for a missing key", "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE_THROWS_AS(reader.get<int>("unknown"), nlohmann::json::out_of_range);
}

TEST_CASE("nlohamReader get throws for a wrong type", "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE_THROWS_AS(reader.get<int>("name"), nlohmann::json::type_error);
}

TEST_CASE("nlohamReader get with fallback returns the stored value",
    "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.get<int>("port", 1234) == 4242);
}

TEST_CASE("nlohamReader get with fallback returns fallback for a missing key",
    "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.get<int>("unknown", 1234) == 1234);
    REQUIRE(reader.get<std::string>("unknown", "default") == "default");
}

TEST_CASE("nlohamReader get with fallback returns fallback for a wrong type",
    "[config][json-reader]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.get<int>("name", 99) == 99);
}

TEST_CASE("nlohamReader get with fallback works on a default reader",
    "[config][json-reader]")
{
    RType::nlohamReader reader;

    REQUIRE(reader.get<int>("port", 4242) == 4242);
}

TEST_CASE("nlohamReader has returns true for an existing key",
    "[config][json-reader][has]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.has("name"));
    REQUIRE(reader.has("port"));
    REQUIRE(reader.has("waves"));
}

TEST_CASE("nlohamReader has returns false for a missing key",
    "[config][json-reader][has]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.has("unknown"));
}

TEST_CASE("nlohamReader has is case sensitive", "[config][json-reader][has]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.has("Name"));
    REQUIRE_FALSE(reader.has("PORT"));
}

TEST_CASE("nlohamReader has only checks top level keys",
    "[config][json-reader][has]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    // "enemy" exists inside waves[0], but not at the root.
    REQUIRE_FALSE(reader.has("enemy"));
    REQUIRE_FALSE(reader.has("count"));
}

TEST_CASE("nlohamReader has returns true for a key holding null",
    "[config][json-reader][has]")
{
    TempFile file(R"({"optional": null})");
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.has("optional"));
}

TEST_CASE("nlohamReader has returns false on a default constructed reader",
    "[config][json-reader][has]")
{
    RType::nlohamReader reader;

    REQUIRE_FALSE(reader.has("anything"));
}

TEST_CASE("nlohamReader has returns false after a failed readFile",
    "[config][json-reader][has]")
{
    TempFile good(LEVEL_JSON);
    TempFile bad("not json");
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(good.path()));
    REQUIRE(reader.has("name"));
    REQUIRE_FALSE(reader.readFile(bad.path()));
    REQUIRE_FALSE(reader.has("name"));
}

TEST_CASE("nlohamReader has returns false when the root is not an object",
    "[config][json-reader][has]")
{
    TempFile file("[1, 2, 3]");
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(file.path()));
    REQUIRE_FALSE(reader.has("name"));
}

TEST_CASE("nlohamReader has does not insert missing keys",
    "[config][json-reader][has]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.has("unknown"));
    REQUIRE_FALSE(reader.has("unknown"));
    REQUIRE_THROWS_AS(reader["unknown"], nlohmann::json::out_of_range);
}

TEST_CASE("nlohamReader has reflects data replaced by a reload",
    "[config][json-reader][has]")
{
    TempFile first(R"({"a": 1})");
    TempFile second(R"({"b": 2})");
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(first.path()));
    REQUIRE(reader.has("a"));
    REQUIRE_FALSE(reader.has("b"));

    REQUIRE(reader.readFile(second.path()));
    REQUIRE_FALSE(reader.has("a"));
    REQUIRE(reader.has("b"));
}

TEST_CASE("nlohamReader validate returns true for a matching struct",
    "[config][json-reader][validate]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate returns false for a missing required key",
    "[config][json-reader][validate]")
{
    TempFile file(R"({"version": 1, "waves": []})");
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate returns false for a wrong type",
    "[config][json-reader][validate]")
{
    TempFile file(R"({"version": "one", "name": "x", "waves": []})");
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate checks nested elements",
    "[config][json-reader][validate]")
{
    TempFile file(R"({
        "version": 1, "name": "x",
        "waves": [ { "time": 1.0, "enemy": "a", "count": 2 },
                   { "time": 2.0, "enemy": "b", "count": "many" } ]
    })");
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate returns false for a missing nested key",
    "[config][json-reader][validate]")
{
    TempFile file(R"({
        "version": 1, "name": "x",
        "waves": [ { "time": 1.0, "count": 2 } ]
    })");
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate accepts an empty array",
    "[config][json-reader][validate]")
{
    TempFile file(R"({"version": 1, "name": "x", "waves": []})");
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate ignores unknown keys",
    "[config][json-reader][validate]")
{
    TempFile file(R"({"version": 1, "name": "x", "waves": [], "enmey": 1})");
    RType::nlohamReader reader(file.path());

    // Documents the known limitation: typos are not detected.
    REQUIRE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate returns false when the root is not an object",
    "[config][json-reader][validate]")
{
    TempFile file("[1, 2, 3]");
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(file.path()));
    REQUIRE_FALSE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate returns false on a default constructed reader",
    "[config][json-reader][validate]")
{
    RType::nlohamReader reader;

    REQUIRE_FALSE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate returns false after a failed readFile",
    "[config][json-reader][validate]")
{
    TempFile good(LEVEL_JSON);
    TempFile bad("not json");
    RType::nlohamReader reader;

    REQUIRE(reader.readFile(good.path()));
    REQUIRE(reader.validate<LevelDef>());
    REQUIRE_FALSE(reader.readFile(bad.path()));
    REQUIRE_FALSE(reader.validate<LevelDef>());
}

TEST_CASE("nlohamReader validate passes with defaults for missing keys",
    "[config][json-reader][validate]")
{
    TempFile file("{}");
    RType::nlohamReader reader(file.path());

    REQUIRE(reader.validate<OptionalSettings>());
}

TEST_CASE("nlohamReader validate does not modify the loaded data",
    "[config][json-reader][validate]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    REQUIRE_FALSE(reader.validate<WaveDef>());
    REQUIRE(reader.has("name"));
    REQUIRE(reader.get<std::string>("name") == "level1");
}

TEST_CASE("nlohamReader convertJson returns a filled struct",
    "[config][json-reader][convert]")
{
    TempFile file(LEVEL_JSON);
    RType::nlohamReader reader(file.path());

    const auto level = reader.convertJson<LevelDef>();

    REQUIRE(level.version == 1);
    REQUIRE(level.name == "level1");
    REQUIRE(level.waves.size() == 2u);
    REQUIRE(level.waves[0].enemy == "bydos_basic");
    REQUIRE(level.waves[0].count == 5);
    REQUIRE(level.waves[0].time == 2.0f);
    REQUIRE(level.waves[1].enemy == "bydos_shooter");
    REQUIRE(level.waves[1].count == 3);
}

TEST_CASE("nlohamReader convertJson throws runtime_error for a missing key",
    "[config][json-reader][convert]")
{
    TempFile file(R"({"version": 1, "waves": []})");
    RType::nlohamReader reader(file.path());

    REQUIRE_THROWS_AS(reader.convertJson<LevelDef>(), std::runtime_error);
}

TEST_CASE("nlohamReader convertJson throws runtime_error for a wrong type",
    "[config][json-reader][convert]")
{
    TempFile file(R"({"version": "one", "name": "x", "waves": []})");
    RType::nlohamReader reader(file.path());

    REQUIRE_THROWS_AS(reader.convertJson<LevelDef>(), std::runtime_error);
}

TEST_CASE("nlohamReader convertJson throws runtime_error for a bad nested element",
    "[config][json-reader][convert]")
{
    TempFile file(R"({
        "version": 1, "name": "x",
        "waves": [ { "time": 1.0, "enemy": "a" } ]
    })");
    RType::nlohamReader reader(file.path());

    REQUIRE_THROWS_AS(reader.convertJson<LevelDef>(), std::runtime_error);
}

TEST_CASE("nlohamReader convertJson uses defaults for lenient structs",
    "[config][json-reader][convert]")
{
    TempFile file(R"({"port": 9000})");
    RType::nlohamReader reader(file.path());

    const auto settings = reader.convertJson<OptionalSettings>();

    REQUIRE(settings.port == 9000);
    REQUIRE_FALSE(settings.autoFire);
}

TEST_CASE("nlohamReader convertJson on an empty object gives all defaults",
    "[config][json-reader][convert]")
{
    TempFile file("{}");
    RType::nlohamReader reader(file.path());

    const auto settings = reader.convertJson<OptionalSettings>();

    REQUIRE(settings.port == 4242);
    REQUIRE_FALSE(settings.autoFire);
}

TEST_CASE("nlohamReader convertJson throws on a default constructed reader",
    "[config][json-reader][convert]")
{
    RType::nlohamReader reader;

    REQUIRE_THROWS_AS(reader.convertJson<LevelDef>(), std::runtime_error);
}

TEST_CASE("nlohamReader convertJson is consistent with validate",
    "[config][json-reader][convert]")
{
    TempFile good(LEVEL_JSON);
    TempFile bad(R"({"version": 1})");
    RType::nlohamReader goodReader(good.path());
    RType::nlohamReader badReader(bad.path());

    REQUIRE(goodReader.validate<LevelDef>());
    REQUIRE_NOTHROW(goodReader.convertJson<LevelDef>());
    REQUIRE_FALSE(badReader.validate<LevelDef>());
    REQUIRE_THROWS(badReader.convertJson<LevelDef>());
}
