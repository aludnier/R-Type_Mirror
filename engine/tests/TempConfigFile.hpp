#ifndef TEMPCONFIGFILE_HPP
#define TEMPCONFIGFILE_HPP

#include <filesystem>
#include <fstream>
#include <string>

class TempConfigFile {
public:
    explicit TempConfigFile(const std::string &contents)
    {
        static std::size_t counter = 0;
        _path = std::filesystem::temp_directory_path() /
            ("rtype_resource_manager_test_" + std::to_string(counter++) +
                ".json");
        _path = std::filesystem::canonical(_path);
        std::ofstream output(_path, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to create temporary config file");
        }
        output << contents;
        if (!output) {
            throw std::runtime_error("Unable to write temporary config file");
        }
    }

    ~TempConfigFile()
    {
        std::error_code error;
        std::filesystem::remove(_path, error);
    }

    [[nodiscard]] const std::filesystem::path &path() const
    {
        return _path;
    }

private:
    std::filesystem::path _path;
};

#endif // TEMPCONFIGFILE_HPP
