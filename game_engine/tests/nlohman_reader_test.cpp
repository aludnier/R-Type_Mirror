#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include "library/NlohmanReader.h"

namespace {
struct WaveDef {
    float time = 0;
    std::string enemy;
    int count = 0;
};

struct LevelDef {
    int version = 0;
    std::string name;
    std::vector<WaveDef> waves;
};

struct OptionalSettings {
    int port = 4242;
    bool autoFire = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WaveDef, time, enemy, count)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LevelDef, version, name, waves)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    OptionalSettings, port, autoFire)

class TempFile {
public:
    explicit TempFile(const std::string &content)
    {
        static int counter = 0;
        _path = std::filesystem::temp_directory_path() /
            ("rtype_reader_test_" + std::to_string(counter++) + ".json");
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

    [[nodiscard]] const std::filesystem::path &path() const
    {
        return _path;
    }

private:
    std::filesystem::path _path;
};

// Temp file + reader already loaded from it.
struct Loaded {
    TempFile file;
    RType::nlohamReader reader;

    explicit Loaded(const std::string &content) :
        file(content), reader(file.path())
    {
    }
};

const std::string LEVEL_JSON = R"({
    "version": 1,
    "name": "level1",
    "port": 4242,
    "volume": 0.5,
    "waves": [
        { "time": 2.0, "enemy": "bydos_basic", "count": 5 },
        { "time": 10.0, "enemy": "bydos_shooter", "count": 3 }
    ]
})";

const std::filesystem::path MISSING =
    std::filesystem::temp_directory_path() / "rtype_reader_missing.json";

// Documents that must NOT convert to a LevelDef.
const std::vector<std::string> INVALID_LEVELS = {
    R"({"version": 1, "waves": []})",
    R"({"version": "one", "name": "x", "waves": []})",
    R"({"version": 1, "name": "x", "waves": [{"time": 1.0, "count": 2}]})",
    R"({"version": 1, "name": "x",
        "waves": [{"time": 1.0, "enemy": "a", "count": "many"}]})",
    R"({"version": 1})",
    "{}",
    "[1, 2, 3]",
};

} // namespace

TEST_CASE("nlohamReader readFile", "[config][json-reader][read]")
{
    RType::nlohamReader reader;

    SECTION("loads a valid file")
    {
        TempFile file(LEVEL_JSON);
        REQUIRE(reader.readFile(file.path()));
        REQUIRE(reader["name"] == "level1");
    }

    SECTION("returns false for a missing file")
    {
        REQUIRE_FALSE(reader.readFile(MISSING));
    }

    SECTION("returns false for malformed or empty content")
    {
        for (const auto *content : {"", "not json", "{ \"version\": 1, "}) {
            TempFile file(content);
            REQUIRE_FALSE(reader.readFile(file.path()));
        }
    }

    SECTION("a failed read clears previously loaded data")
    {
        TempFile good(LEVEL_JSON);
        TempFile bad("not json");
        REQUIRE(reader.readFile(good.path()));
        REQUIRE_FALSE(reader.readFile(bad.path()));
        REQUIRE_THROWS_AS(reader["name"], nlohmann::json::out_of_range);
    }

    SECTION("a reload replaces the data")
    {
        TempFile first(R"({"name": "first"})");
        TempFile second(R"({"name": "second"})");
        REQUIRE(reader.readFile(first.path()));
        REQUIRE(reader["name"] == "first");
        REQUIRE(reader.readFile(second.path()));
        REQUIRE(reader["name"] == "second");
    }
}

TEST_CASE("nlohamReader constructor", "[config][json-reader][ctor]")
{
    SECTION("loads a valid file")
    {
        TempFile file(LEVEL_JSON);
        REQUIRE_NOTHROW(RType::nlohamReader(file.path()));
    }

    SECTION("throws for a missing file")
    {
        REQUIRE_THROWS_AS(RType::nlohamReader(MISSING), std::runtime_error);
    }

    SECTION("throws for malformed JSON")
    {
        TempFile file("{ broken");
        REQUIRE_THROWS_AS(RType::nlohamReader(file.path()), std::runtime_error);
    }
}

TEST_CASE("nlohamReader operator[]", "[config][json-reader][index]")
{
    SECTION("a default reader has no data")
    {
        RType::nlohamReader reader;
        REQUIRE_THROWS_AS(reader["anything"], nlohmann::json::out_of_range);
    }

    SECTION("throws for a missing key")
    {
        Loaded l(LEVEL_JSON);
        REQUIRE_THROWS_AS(l.reader["unknown"], nlohmann::json::out_of_range);
    }

    SECTION("allows chained access")
    {
        Loaded l(LEVEL_JSON);
        auto &reader = l.reader;

        REQUIRE(reader["waves"].size() == 2u);
        REQUIRE(reader["waves"].at(0).at("enemy") == "bydos_basic");
        REQUIRE(reader["waves"].at(1).at("count") == 3);
        REQUIRE_THROWS_AS(
            reader["waves"].at(0).at("missing"), nlohmann::json::out_of_range);
        REQUIRE_THROWS_AS(reader["waves"].at(5), nlohmann::json::out_of_range);
    }
}

TEST_CASE("nlohamReader get", "[config][json-reader][get]")
{
    Loaded l(LEVEL_JSON);
    auto &reader = l.reader;

    SECTION("returns typed values")
    {
        REQUIRE(reader.get<int>("port") == 4242);
        REQUIRE(reader.get<std::string>("name") == "level1");
        REQUIRE(reader.get<float>("volume") == 0.5f);
    }

    SECTION("throws for a missing key or a wrong type")
    {
        REQUIRE_THROWS_AS(
            reader.get<int>("unknown"), nlohmann::json::out_of_range);
        REQUIRE_THROWS_AS(reader.get<int>("name"), nlohmann::json::type_error);
    }

    SECTION("with fallback: stored value wins")
    {
        REQUIRE(reader.get<int>("port", 1234) == 4242);
    }

    SECTION("with fallback: used for a missing key or a wrong type")
    {
        REQUIRE(reader.get<int>("unknown", 1234) == 1234);
        REQUIRE(reader.get<std::string>("unknown", "default") == "default");
        REQUIRE(reader.get<int>("name", 99) == 99);
    }

    SECTION("with fallback: works on a default reader")
    {
        RType::nlohamReader empty;
        REQUIRE(empty.get<int>("port", 4242) == 4242);
    }
}

TEST_CASE("nlohamReader has", "[config][json-reader][has]")
{
    SECTION("true for existing top level keys, false otherwise")
    {
        Loaded l(LEVEL_JSON);
        REQUIRE(l.reader.has("name"));
        REQUIRE(l.reader.has("port"));
        REQUIRE(l.reader.has("waves"));
        REQUIRE_FALSE(l.reader.has("unknown"));
        REQUIRE_FALSE(l.reader.has("Name")); // case sensitive
        REQUIRE_FALSE(l.reader.has("enemy")); // nested keys are not found
    }

    SECTION("true for a key holding null")
    {
        Loaded l(R"({"optional": null})");
        REQUIRE(l.reader.has("optional"));
    }

    SECTION("false on a default reader")
    {
        RType::nlohamReader reader;
        REQUIRE_FALSE(reader.has("anything"));
    }

    SECTION("false when the root is not an object")
    {
        Loaded l("[1, 2, 3]");
        REQUIRE_FALSE(l.reader.has("name"));
    }

    SECTION("does not insert missing keys")
    {
        Loaded l(LEVEL_JSON);
        REQUIRE_FALSE(l.reader.has("unknown"));
        REQUIRE_FALSE(l.reader.has("unknown"));
        REQUIRE_THROWS_AS(l.reader["unknown"], nlohmann::json::out_of_range);
    }

    SECTION("reflects a failed read and a reload")
    {
        TempFile first(R"({"a": 1})");
        TempFile second(R"({"b": 2})");
        TempFile bad("not json");
        RType::nlohamReader reader;

        REQUIRE(reader.readFile(first.path()));
        REQUIRE(reader.has("a"));
        REQUIRE_FALSE(reader.has("b"));

        REQUIRE(reader.readFile(second.path()));
        REQUIRE_FALSE(reader.has("a"));
        REQUIRE(reader.has("b"));

        REQUIRE_FALSE(reader.readFile(bad.path()));
        REQUIRE_FALSE(reader.has("b"));
    }
}

TEST_CASE("nlohamReader validate", "[config][json-reader][validate]")
{
    SECTION("true for matching structs")
    {
        Loaded l(LEVEL_JSON);
        REQUIRE(l.reader.validate<LevelDef>());

        Loaded empty(R"({"version": 1, "name": "x", "waves": []})");
        REQUIRE(empty.reader.validate<LevelDef>());
    }

    SECTION("false for every invalid document")
    {
        for (const auto &content : INVALID_LEVELS) {
            Loaded l(content);
            INFO(content);
            REQUIRE_FALSE(l.reader.validate<LevelDef>());
        }
    }

    SECTION("known limitation: unknown keys are ignored")
    {
        Loaded l(R"({"version": 1, "name": "x", "waves": [], "enmey": 1})");
        REQUIRE(l.reader.validate<LevelDef>());
    }

    SECTION("false on a default reader")
    {
        RType::nlohamReader reader;
        REQUIRE_FALSE(reader.validate<LevelDef>());
    }

    SECTION("false after a failed readFile")
    {
        TempFile good(LEVEL_JSON);
        TempFile bad("not json");
        RType::nlohamReader reader;

        REQUIRE(reader.readFile(good.path()));
        REQUIRE(reader.validate<LevelDef>());
        REQUIRE_FALSE(reader.readFile(bad.path()));
        REQUIRE_FALSE(reader.validate<LevelDef>());
    }

    SECTION("defaults make an empty object valid for lenient structs")
    {
        Loaded l("{}");
        REQUIRE(l.reader.validate<OptionalSettings>());
    }

    SECTION("does not modify the loaded data")
    {
        Loaded l(LEVEL_JSON);
        REQUIRE_FALSE(l.reader.validate<WaveDef>());
        REQUIRE(l.reader.has("name"));
        REQUIRE(l.reader.get<std::string>("name") == "level1");
    }
}

TEST_CASE("nlohamReader convertJson", "[config][json-reader][convert]")
{
    SECTION("returns a filled struct")
    {
        Loaded l(LEVEL_JSON);
        const auto level = l.reader.convertJson<LevelDef>();

        REQUIRE(level.version == 1);
        REQUIRE(level.name == "level1");
        REQUIRE(level.waves.size() == 2u);
        REQUIRE(level.waves[0].enemy == "bydos_basic");
        REQUIRE(level.waves[0].count == 5);
        REQUIRE(level.waves[0].time == 2.0f);
        REQUIRE(level.waves[1].enemy == "bydos_shooter");
        REQUIRE(level.waves[1].count == 3);
    }

    SECTION("throws runtime_error for every invalid document")
    {
        for (const auto &content : INVALID_LEVELS) {
            Loaded l(content);
            INFO(content);
            REQUIRE_THROWS_AS(
                l.reader.convertJson<LevelDef>(), std::runtime_error);
        }
    }

    SECTION("throws on a default reader")
    {
        RType::nlohamReader reader;
        REQUIRE_THROWS_AS(reader.convertJson<LevelDef>(), std::runtime_error);
    }

    SECTION("uses defaults for lenient structs")
    {
        Loaded partial(R"({"port": 9000})");
        const auto settings = partial.reader.convertJson<OptionalSettings>();
        REQUIRE(settings.port == 9000);
        REQUIRE_FALSE(settings.autoFire);

        Loaded empty("{}");
        const auto defaults = empty.reader.convertJson<OptionalSettings>();
        REQUIRE(defaults.port == 4242);
        REQUIRE_FALSE(defaults.autoFire);
    }

    SECTION("is consistent with validate")
    {
        Loaded good(LEVEL_JSON);
        Loaded bad(R"({"version": 1})");

        REQUIRE(good.reader.validate<LevelDef>());
        REQUIRE_NOTHROW(good.reader.convertJson<LevelDef>());
        REQUIRE_FALSE(bad.reader.validate<LevelDef>());
        REQUIRE_THROWS(bad.reader.convertJson<LevelDef>());
    }
}
