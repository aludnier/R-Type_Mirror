/*
** EPITECH PROJECT, 2026
** Rtype
** File description:
** nlohamm reader
*/

#include "NlohmanReader.h"
#include <exception>
#include <fstream>

RType::nlohamReader::nlohamReader(const std::filesystem::path &path) :
    _file(path)
{
    if (!readFile(path)) {
        throw std::runtime_error(
            "[NlohmanReader] cannot read config file '" + _file.string() + "'");
    }
}

bool RType::nlohamReader::readFile(const std::filesystem::path &path)
{
    _file = path;
    _data = nlohmann::json::object();
    if (!std::filesystem::exists(_file)) {
        return false;
    }
    std::ifstream f(path);
    if (!f) {
        return false;
    }
    try {
        _data = nlohmann::json::parse(f);
    } catch (const nlohmann::json::parse_error &_) {
        return false;
    }

    f.close();
    return true;
}

nlohmann::json RType::nlohamReader::operator[](const std::string &param)
{
    return _data.at(param);
}

bool RType::nlohamReader::has(const std::string &param)
{
    return _data.contains(param);
}
